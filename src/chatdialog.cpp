/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Zhenkai Zhu <zhenkai@cs.ucla.edu>
 *         Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *         Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "chatdialog.h"
#include "ui_chatdialog.h"

#include <QScrollBar>
#include <QMessageBox>
#include <QCloseEvent>

#ifndef Q_MOC_RUN
#include <ndn.cxx/security/identity/identity-manager.h>
#include <ndn.cxx/security/policy/no-verify-policy-manager.h>
#include <ndn.cxx/security/encryption/basic-encryption-manager.h>
#include <sync-intro-certificate.h>
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include "logging.h"
#endif

using namespace std;

INIT_LOGGER("ChatDialog");

static const int HELLO_INTERVAL = FRESHNESS * 3 / 4;

Q_DECLARE_METATYPE(std::vector<Sync::MissingDataInfo> )
Q_DECLARE_METATYPE(size_t)

ChatDialog::ChatDialog(ndn::Ptr<ContactManager> contactManager,
                       const ndn::Name& chatroomPrefix,
		       const ndn::Name& localPrefix,
                       const ndn::Name& defaultIdentity,
                       const std::string& nick,
                       bool trial,
		       QWidget *parent) 
: QDialog(parent)
  , ui(new Ui::ChatDialog)
  , m_contactManager(contactManager)
  , m_chatroomPrefix(chatroomPrefix)
  , m_localPrefix(localPrefix)
  , m_defaultIdentity(defaultIdentity)
  , m_invitationPolicyManager(ndn::Ptr<InvitationPolicyManager>(new InvitationPolicyManager(m_chatroomPrefix.get(-1).toUri(), m_defaultIdentity)))
  , m_nick(nick)
  , m_sock(NULL)
  , m_lastMsgTime(0)
  // , m_historyInitialized(false)
  , m_joined(false)
  , m_inviteListDialog(new InviteListDialog(m_contactManager))
{
  qRegisterMetaType<std::vector<Sync::MissingDataInfo> >("std::vector<Sync::MissingDataInfo>");
  qRegisterMetaType<size_t>("size_t");

  ui->setupUi(this);

  QString randString = getRandomString();
  m_localChatPrefix = m_localPrefix;
  m_localChatPrefix.append("%F0.").append(m_defaultIdentity);
  m_localChatPrefix.append("chronos").append(m_chatroomPrefix.get(-1)).append(randString.toStdString());

  m_session = time(NULL);
  m_scene = new DigestTreeScene(this);

  initializeSetting();
  updateLabels();

  ui->treeViewer->setScene(m_scene);
  ui->treeViewer->hide();
  m_scene->plot("Empty");
  QRectF rect = m_scene->itemsBoundingRect();
  m_scene->setSceneRect(rect);

  m_rosterModel = new QStringListModel(this);
  ui->listView->setModel(m_rosterModel);

  createActions();
  createTrayIcon();

  m_timer = new QTimer(this);

  setWrapper(trial);

  connect(ui->inviteButton, SIGNAL(clicked()),
          this, SLOT(openInviteListDialog()));
  connect(m_inviteListDialog, SIGNAL(invitionDetermined(QString, bool)),
          this, SLOT(sendInvitationWrapper(QString, bool)));
  connect(ui->lineEdit, SIGNAL(returnPressed()), 
          this, SLOT(returnPressed()));
  connect(ui->treeButton, SIGNAL(pressed()), 
          this, SLOT(treeButtonPressed()));
  connect(this, SIGNAL(dataReceived(QString, const char *, size_t, bool, bool)), 
          this, SLOT(processData(QString, const char *, size_t, bool, bool)));
  connect(this, SIGNAL(treeUpdated(const std::vector<Sync::MissingDataInfo>)), 
          this, SLOT(processTreeUpdate(const std::vector<Sync::MissingDataInfo>)));
  connect(m_timer, SIGNAL(timeout()), 
          this, SLOT(replot()));
  connect(m_scene, SIGNAL(replot()), 
          this, SLOT(replot()));
  connect(trayIcon, SIGNAL(messageClicked()), 
          this, SLOT(showNormal()));
  connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), 
          this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
  connect(m_scene, SIGNAL(rosterChanged(QStringList)), 
          this, SLOT(updateRosterList(QStringList)));


  initializeSync();
}


ChatDialog::~ChatDialog()
{
  if(m_sock != NULL)
    {
      sendLeave();
      delete m_sock;
      m_sock = NULL;
    }
  m_handler->shutdown();
}

void
ChatDialog::setWrapper(bool trial)
{
  m_identityManager = ndn::Ptr<ndn::security::IdentityManager>::Create();

  ndn::Name certificateName = m_identityManager->getDefaultCertificateNameByIdentity(m_defaultIdentity);
  m_syncPolicyManager = ndn::Ptr<SyncPolicyManager>(new SyncPolicyManager(m_defaultIdentity, certificateName, m_chatroomPrefix));

  m_keychain = ndn::Ptr<ndn::security::Keychain>(new ndn::security::Keychain(m_identityManager, m_invitationPolicyManager, NULL));

  ndn::Ptr<ndn::security::Keychain> noVerifyKeychain = ndn::Ptr<ndn::security::Keychain>(new ndn::security::Keychain(m_identityManager,
ndn::Ptr<ndn::security::NoVerifyPolicyManager>::Create(), NULL));
  try{
    m_handler = ndn::Ptr<ndn::Wrapper>(new ndn::Wrapper(m_keychain));
    m_localPrefixHandler = ndn::Ptr<ndn::Wrapper>(new ndn::Wrapper(noVerifyKeychain));
  }catch(ndn::Error::ndnOperation& e){
    emit noNdnConnection(QString::fromStdString("Cannot conect to ndnd!\n Have you started your ndnd?"));
  }
}

void
ChatDialog::initializeSetting()
{
  m_user.setNick(QString::fromStdString(m_nick));
  m_user.setChatroom(QString::fromStdString(m_chatroomPrefix.get(-1).toUri()));
  m_user.setOriginPrefix(QString::fromStdString(m_localPrefix.toUri()));
  m_user.setPrefix(QString::fromStdString(m_localChatPrefix.toUri()));
  m_scene->setCurrentPrefix(QString::fromStdString(m_localChatPrefix.toUri()));
}

void
ChatDialog::updateLabels()
{
  QString settingDisp = QString("Chatroom: %1").arg(m_user.getChatroom());
  ui->infoLabel->setStyleSheet("QLabel {color: #630; font-size: 16px; font: bold \"Verdana\";}");
  ui->infoLabel->setText(settingDisp);
  QString prefixDisp;
  if (m_user.getPrefix().startsWith("/private/local"))
  {
    prefixDisp = QString("<Warning: no connection to hub or hub does not support prefix autoconfig.>\n <Prefix = %1>").arg(m_user.getPrefix());
    ui->prefixLabel->setStyleSheet("QLabel {color: red; font-size: 12px; font: bold \"Verdana\";}");
  }
  else
  {
    prefixDisp = QString("<Prefix = %1>").arg(m_user.getPrefix());
    ui->prefixLabel->setStyleSheet("QLabel {color: Green; font-size: 12px; font: bold \"Verdana\";}");
  }
  ui->prefixLabel->setText(prefixDisp);
}

void
ChatDialog::sendInvitation(ndn::Ptr<ContactItem> contact, bool isIntroducer)
{
  m_invitationPolicyManager->addTrustAnchor(contact->getSelfEndorseCertificate());

  ndn::Name certificateName = m_identityManager->getDefaultCertificateNameByIdentity(m_defaultIdentity);

  ndn::Name interestName("/ndn/broadcast/chronos/invitation");
  interestName.append(contact->getNameSpace());
  interestName.append("chatroom");
  interestName.append(m_chatroomPrefix.get(-1));
  interestName.append("inviter-prefix");
  interestName.append(m_localPrefix);
  interestName.append("inviter");
  interestName.append(certificateName);

  string signedUri = interestName.toUri();
  ndn::Blob signedBlob(signedUri.c_str(), signedUri.size());

  ndn::Ptr<const ndn::signature::Sha256WithRsa> sha256sig = ndn::DynamicCast<const ndn::signature::Sha256WithRsa>(m_identityManager->signByCertificate(signedBlob, certificateName));
  const ndn::Blob& sigBits = sha256sig->getSignatureBits();

  interestName.append(sigBits.buf(), sigBits.size());
  interestName.appendVersion();

  ndn::Ptr<ndn::Interest> interest = ndn::Ptr<ndn::Interest>(new ndn::Interest(interestName));
  ndn::Ptr<ndn::Closure> closure = ndn::Ptr<ndn::Closure>(new ndn::Closure(boost::bind(&ChatDialog::onInviteReplyVerified,
                                                                                       this,
                                                                                       _1,
                                                                                       contact->getNameSpace(),
                                                                                       isIntroducer),
                                                                           boost::bind(&ChatDialog::onInviteTimeout,
                                                                                       this,
                                                                                       _1,
                                                                                       _2,
                                                                                       contact->getNameSpace(),
                                                                                       7),
                                                                           boost::bind(&ChatDialog::onUnverified,
                                                                                       this,
                                                                                       _1)));

  m_handler->sendInterest(interest, closure);
}

void
ChatDialog::addTrustAnchor(const EndorseCertificate& selfEndorseCertificate)
{ m_invitationPolicyManager->addTrustAnchor(selfEndorseCertificate); }

void
ChatDialog::addChatDataRule(const ndn::Name& prefix, 
                            const ndn::security::IdentityCertificate& identityCertificate,
                            bool isIntroducer)
{ m_syncPolicyManager->addChatDataRule(prefix, identityCertificate, isIntroducer); }

void
ChatDialog::publishIntroCert(const ndn::security::IdentityCertificate& dskCertificate, bool isIntroducer)
{
  SyncIntroCertificate syncIntroCertificate(m_chatroomPrefix,
                                            dskCertificate.getPublicKeyName(),
                                            m_identityManager->getDefaultKeyNameForIdentity(m_defaultIdentity),
                                            dskCertificate.getNotBefore(),
                                            dskCertificate.getNotAfter(),
                                            dskCertificate.getPublicKeyInfo(),
                                            (isIntroducer ? SyncIntroCertificate::INTRODUCER : SyncIntroCertificate::PRODUCER));
  ndn::Name certName = m_identityManager->getDefaultCertificateNameByIdentity(m_defaultIdentity);
  _LOG_DEBUG("Publish Intro Certificate: " << syncIntroCertificate.getName());
  m_identityManager->signByCertificate(syncIntroCertificate, certName);
  m_handler->putToNdnd(*syncIntroCertificate.encodeToWire());
}

void
ChatDialog::invitationRejected(const ndn::Name& identity)
{
  _LOG_DEBUG(" " << identity.toUri() << " Rejected your invitation!");
  QString msg = QString::fromStdString(identity.toUri()) + " Rejected your invitation!";
  emit inivationRejection(msg);
}

void
ChatDialog::invitationAccepted(const ndn::Name& identity, ndn::Ptr<ndn::Data> data, const string& inviteePrefix, bool isIntroducer)
{
  _LOG_DEBUG(" " << identity.toUri() << " Accepted your invitation!");
  ndn::Ptr<const ndn::signature::Sha256WithRsa> sha256sig = boost::dynamic_pointer_cast<const ndn::signature::Sha256WithRsa> (data->getSignature());
  const ndn::Name & keyLocatorName = sha256sig->getKeyLocator().getKeyName();
  ndn::Ptr<ndn::security::IdentityCertificate> dskCertificate = m_invitationPolicyManager->getValidatedDskCertificate(keyLocatorName);
  m_syncPolicyManager->addChatDataRule(inviteePrefix, *dskCertificate, isIntroducer);
  publishIntroCert(*dskCertificate, isIntroducer);
}

void 
ChatDialog::onInviteReplyVerified(ndn::Ptr<ndn::Data> data, const ndn::Name& identity, bool isIntroducer)
{
  string content(data->content().buf(), data->content().size());
  if(content == string("nack"))
    invitationRejected(identity);
  else
    invitationAccepted(identity, data, content, isIntroducer);
}

void 
ChatDialog::onInviteTimeout(ndn::Ptr<ndn::Closure> closure, ndn::Ptr<ndn::Interest> interest, const ndn::Name& identity, int retry)
{
  if(retry > 0)
    {
      ndn::Ptr<ndn::Closure> newClosure = ndn::Ptr<ndn::Closure>(new ndn::Closure(closure->m_dataCallback,
                                                                                  boost::bind(&ChatDialog::onInviteTimeout, 
                                                                                              this, 
                                                                                              _1, 
                                                                                              _2, 
                                                                                              identity,
                                                                                              retry - 1),
                                                                                  closure->m_unverifiedCallback,
                                                                                  closure->m_stepCount)
                                                                 );
      m_handler->sendInterest(interest, newClosure);
    }
  else
    invitationRejected(identity);
}
 
void
ChatDialog::onUnverified(ndn::Ptr<ndn::Data> data)
{}

void
ChatDialog::onTimeout(ndn::Ptr<ndn::Closure> closure, 
                      ndn::Ptr<ndn::Interest> interest)
{}

void
ChatDialog::initializeSync()
{
  
  m_sock = new Sync::SyncSocket(m_chatroomPrefix.toUri(),
                                m_syncPolicyManager,
                                bind(&ChatDialog::processTreeUpdateWrapper, this, _1, _2),
                                bind(&ChatDialog::processRemoveWrapper, this, _1));
  
  usleep(100000);

  QTimer::singleShot(600, this, SLOT(sendJoin()));
  m_timer->start(FRESHNESS * 1000);
  disableTreeDisplay();
  QTimer::singleShot(2200, this, SLOT(enableTreeDisplay()));
  // Sync::CcnxWrapperPtr handle = boost::make_shared<Sync::CcnxWrapper> ();
  // handle->setInterestFilter(m_user.getPrefix().toStdString(), bind(&ChatDialog::respondHistoryRequest, this, _1));
}

void
ChatDialog::returnPressed()
{
  QString text = ui->lineEdit->text();
  if (text.isEmpty())
    return;

  ui->lineEdit->clear();

  if (text.startsWith("boruoboluomi"))
  {
    summonReaper ();
    // reapButton->show();
    fitView();
    return;
  }

  if (text.startsWith("minimanihong"))
  {
    // reapButton->hide();
    fitView();
    return;
  }

  SyncDemo::ChatMessage msg;
  formChatMessage(text, msg);

  appendMessage(msg);

  sendMsg(msg);

  fitView();
}

void 
ChatDialog::treeButtonPressed()
{
  if (ui->treeViewer->isVisible())
  {
    ui->treeViewer->hide();
    ui->treeButton->setText("Show ChronoSync Tree");
  }
  else
  {
    ui->treeViewer->show();
    ui->treeButton->setText("Hide ChronoSync Tree");
  }

  fitView();
}

void ChatDialog::disableTreeDisplay()
{
  ui->treeButton->setEnabled(false);
  ui->treeViewer->hide();
  fitView();
}

void ChatDialog::enableTreeDisplay()
{
  ui->treeButton->setEnabled(true);
  // treeViewer->show();
  // fitView();
}

void
ChatDialog::processTreeUpdateWrapper(const std::vector<Sync::MissingDataInfo> v, Sync::SyncSocket *sock)
{
  emit treeUpdated(v);
  _LOG_DEBUG("<<< Tree update signal emitted");
}

void
ChatDialog::processRemoveWrapper(std::string prefix)
{
  _LOG_DEBUG("Sync REMOVE signal received for prefix: " << prefix);
}

void
ChatDialog::processTreeUpdate(const std::vector<Sync::MissingDataInfo> v)
{
  _LOG_DEBUG("<<< processing Tree Update");

  if (v.empty())
  {
    return;
  }

  // reflect the changes on digest tree
  {
    boost::recursive_mutex::scoped_lock lock(m_sceneMutex);
    m_scene->processUpdate(v, m_sock->getRootDigest().c_str());
  }

  int n = v.size();
  int totalMissingPackets = 0;
  for (int i = 0; i < n; i++)
  {
    totalMissingPackets += v[i].high.getSeq() - v[i].low.getSeq() + 1;
  }

  for (int i = 0; i < n; i++)
  {
    if (totalMissingPackets < 4)
    {
      for (Sync::SeqNo seq = v[i].low; seq <= v[i].high; ++seq)
      {
        m_sock->fetchData(v[i].prefix, seq, bind(&ChatDialog::processDataWrapper, this, _1), 2);
        _LOG_DEBUG("<<< Fetching " << v[i].prefix << "/" <<seq.getSession() <<"/" << seq.getSeq());
      }
    }
    else
    {
        m_sock->fetchData(v[i].prefix, v[i].high, bind(&ChatDialog::processDataNoShowWrapper, this, _1), 2);
    }
  }

  // adjust the view
  fitView();

}

void
ChatDialog::processDataWrapper(ndn::Ptr<ndn::Data> data)
{
  string name = data->getName().toUri();
  const char* buf = data->content().buf();
  size_t len = data->content().size();

  char *tempBuf = new char[len];
  memcpy(tempBuf, buf, len);
  emit dataReceived(name.c_str(), tempBuf, len, true, false);
  _LOG_DEBUG("<<< " << name << " fetched");
}

void
ChatDialog::processDataNoShowWrapper(ndn::Ptr<ndn::Data> data)
{
  string name = data->getName().toUri();
  const char* buf = data->content().buf();
  size_t len = data->content().size();

  char *tempBuf = new char[len];
  memcpy(tempBuf, buf, len);
  emit dataReceived(name.c_str(), tempBuf, len, false, false);

  // if (!m_historyInitialized)
  // {
  //   fetchHistory(name);
  //   m_historyInitialized = true;
  // }
}

// void
// ChatDialog::fetchHistory(std::string name)
// {

//   /****************************/
//   /* TODO: fix following part */
//   /****************************/
//   string nameWithoutSeq = name.substr(0, name.find_last_of('/'));
//   string prefix = nameWithoutSeq.substr(0, nameWithoutSeq.find_last_of('/'));
//   prefix += "/history";
//   // Ptr<Wrapper>CcnxWrapperPtr handle = boost::make_shared<Sync::CcnxWrapper> ();
//   // QString randomString = getRandomString();
//   // for (int i = 0; i < MAX_HISTORY_ENTRY; i++)
//   // {
//   //   QString interest = QString("%1/%2/%3").arg(prefix.c_str()).arg(randomString).arg(i);
//   //   handle->sendInterest(interest.toStdString(), bind(&ChatDialog::processDataHistoryWrapper, this, _1, _2, _3));
//   // }
// }

void
ChatDialog::processData(QString name, const char *buf, size_t len, bool show, bool isHistory)
{
  SyncDemo::ChatMessage msg;
  bool corrupted = false;
  if (!msg.ParseFromArray(buf, len))
  {
    _LOG_DEBUG("Errrrr.. Can not parse msg with name: " << name.toStdString() << ". what is happening?");
    // nasty stuff: as a remedy, we'll form some standard msg for inparsable msgs
    msg.set_from("inconnu");
    msg.set_type(SyncDemo::ChatMessage::OTHER);
    corrupted = true;
  }

  delete [] buf;
  buf = NULL;

  // display msg received from network
  // we have to do so; this function is called by ccnd thread
  // so if we call appendMsg directly
  // Qt crash as "QObject: Cannot create children for a parent that is in a different thread"
  // the "cannonical" way to is use signal-slot
  if (show && !corrupted)
  {
    appendMessage(msg, isHistory);
  }

  if (!isHistory)
  {
    // update the tree view
    std::string stdStrName = name.toStdString();
    std::string stdStrNameWithoutSeq = stdStrName.substr(0, stdStrName.find_last_of('/'));
    std::string prefix = stdStrNameWithoutSeq.substr(0, stdStrNameWithoutSeq.find_last_of('/'));
    _LOG_DEBUG("<<< updating scene for" << prefix << ": " << msg.from());
    if (msg.type() == SyncDemo::ChatMessage::LEAVE)
    {
      processRemove(prefix.c_str());
    }
    else
    {
      boost::recursive_mutex::scoped_lock lock(m_sceneMutex);
      m_scene->msgReceived(prefix.c_str(), msg.from().c_str());
    }
  }
  fitView();
}

void
ChatDialog::processRemove(QString prefix)
{
  _LOG_DEBUG("<<< remove node for prefix" << prefix.toStdString());

  bool removed = m_scene->removeNode(prefix);
  if (removed)
  {
    boost::recursive_mutex::scoped_lock lock(m_sceneMutex);
    m_scene->plot(m_sock->getRootDigest().c_str());
  }
}

void
ChatDialog::sendJoin()
{
  m_joined = true;
  SyncDemo::ChatMessage msg;
  formControlMessage(msg, SyncDemo::ChatMessage::JOIN);
  sendMsg(msg);
  boost::random::random_device rng;
  boost::random::uniform_int_distribution<> uniform(1, FRESHNESS / 5 * 1000);
  m_randomizedInterval = HELLO_INTERVAL * 1000 + uniform(rng);
  QTimer::singleShot(m_randomizedInterval, this, SLOT(sendHello()));
}

void
ChatDialog::sendHello()
{
  time_t now = time(NULL);
  int elapsed = now - m_lastMsgTime;
  if (elapsed >= m_randomizedInterval / 1000)
  {
    SyncDemo::ChatMessage msg;
    formControlMessage(msg, SyncDemo::ChatMessage::HELLO);
    sendMsg(msg);
    boost::random::random_device rng;
    boost::random::uniform_int_distribution<> uniform(1, FRESHNESS / 5 * 1000);
    m_randomizedInterval = HELLO_INTERVAL * 1000 + uniform(rng);
    QTimer::singleShot(m_randomizedInterval, this, SLOT(sendHello()));
  }
  else
  {
    QTimer::singleShot((m_randomizedInterval - elapsed * 1000), this, SLOT(sendHello()));
  }
}

void
ChatDialog::sendLeave()
{
  SyncDemo::ChatMessage msg;
  formControlMessage(msg, SyncDemo::ChatMessage::LEAVE);
  sendMsg(msg);
  usleep(500000);
  m_sock->remove(m_user.getPrefix().toStdString());
  usleep(5000);
  m_joined = false;
  _LOG_DEBUG("Sync REMOVE signal sent");
}

void
ChatDialog::replot()
{
  boost::recursive_mutex::scoped_lock lock(m_sceneMutex);
  m_scene->plot(m_sock->getRootDigest().c_str());
  fitView();
}

void
ChatDialog::summonReaper()
{
  Sync::SyncLogic &logic = m_sock->getLogic ();
  map<string, bool> branches = logic.getBranchPrefixes();
  QMap<QString, DisplayUserPtr> roster = m_scene->getRosterFull();

  m_zombieList.clear();

  QMapIterator<QString, DisplayUserPtr> it(roster);
  map<string, bool>::iterator mapIt;
  while(it.hasNext())
  {
    it.next();
    DisplayUserPtr p = it.value();
    if (p != DisplayUserNullPtr)
    {
      mapIt = branches.find(p->getPrefix().toStdString());
      if (mapIt != branches.end())
      {
        mapIt->second = true;
      }
    }
  }

  for (mapIt = branches.begin(); mapIt != branches.end(); ++mapIt)
  {
    // this is zombie. all active users should have been marked true
    if (! mapIt->second)
    {
      m_zombieList.append(mapIt->first.c_str());
    }
  }

  m_zombieIndex = 0;

  // start reaping
  reap();
}

void
ChatDialog::reap()
{
  if (m_zombieIndex < m_zombieList.size())
  {
    string prefix = m_zombieList.at(m_zombieIndex).toStdString();
    m_sock->remove(prefix);
    _LOG_DEBUG("Reaped: prefix = " << prefix);
    m_zombieIndex++;
    // reap again in 10 seconds
    QTimer::singleShot(10000, this, SLOT(reap()));
  }
}

void
ChatDialog::updateRosterList(QStringList staleUserList)
{
  boost::recursive_mutex::scoped_lock lock(m_sceneMutex);
  QStringList rosterList = m_scene->getRosterList();
  m_rosterModel->setStringList(rosterList);
  QString user;
  QStringListIterator it(staleUserList);
  while(it.hasNext())
  {
    std::string nick = it.next().toStdString();
    if (nick.empty())
      continue;

    SyncDemo::ChatMessage msg;
    formControlMessage(msg, SyncDemo::ChatMessage::LEAVE);
    msg.set_from(nick);
    appendMessage(msg);
  }
}

void
ChatDialog::settingUpdated(QString nick, QString chatroom, QString originPrefix)
{
  _LOG_DEBUG("called");
  QString randString = getRandomString();
  bool needWrite = false;
  bool needFresh = false;

  QString oldPrefix = m_user.getPrefix();
  if (!originPrefix.isEmpty() && originPrefix != m_user.getOriginPrefix()) {
    m_user.setOriginPrefix(originPrefix);

    m_localPrefix = ndn::Name(originPrefix.toStdString());
    m_localChatPrefix = m_localPrefix;
    m_localChatPrefix.append("%F0.").append(m_defaultIdentity);
    m_localChatPrefix.append("chronos").append(m_chatroomPrefix.get(-1)).append(randString.toStdString());
    m_user.setPrefix(QString::fromStdString(m_localChatPrefix.toUri()));
    m_scene->setCurrentPrefix(QString::fromStdString(m_localChatPrefix.toUri()));
    needWrite = true;
    needFresh = true;
  }

  if (needWrite) {
    updateLabels();
  }

  if (needFresh && m_sock != NULL)
  {

    {
      boost::recursive_mutex::scoped_lock lock(m_sceneMutex);
      m_scene->clearAll();
      m_scene->plot("Empty");
    }

    ui->textEdit->clear();

    // keep the new prefix
    QString newPrefix = m_user.getPrefix();
    // send leave for the old
    m_user.setPrefix(oldPrefix);
    // there is no point to send leave if we haven't joined yet
    if (m_joined)
    {
      sendLeave();
    }
    // resume new prefix
    m_user.setPrefix(newPrefix);
    // Sync::CcnxWrapperPtr handle = Sync::CcnxWrapper::Create();
    // handle->clearInterestFilter(oldPrefix.toStdString());
    delete m_sock;
    m_sock = NULL;

    try
    {
      usleep(100000);
      m_sock = new Sync::SyncSocket(m_chatroomPrefix.toUri(),
                                    m_syncPolicyManager,
                                    bind(&ChatDialog::processTreeUpdateWrapper, this, _1, _2),
                                    bind(&ChatDialog::processRemoveWrapper, this, _1));
      usleep(100000);
      // Sync::CcnxWrapperPtr handle = boost::make_shared<Sync::CcnxWrapper> ();
      // handle->setInterestFilter(m_user.getPrefix().toStdString(), bind(&ChatDialog::respondHistoryRequest, this, _1));
      QTimer::singleShot(600, this, SLOT(sendJoin()));
      m_timer->start(FRESHNESS * 1000);
      disableTreeDisplay();
      QTimer::singleShot(2200, this, SLOT(enableTreeDisplay()));
    }catch(ndn::Error::ndnOperation& e){
      emit noNdnConnection(QString::fromStdString("Cannot conect to ndnd!\n Have you started your ndnd?"));
    }
  }
  else if (needFresh && m_sock == NULL)
  {
    initializeSync();
  }
  else if (m_sock == NULL)
  {
    initializeSync();
  }
  else
  {
// #ifdef __DEBUG
//     std::cout << "Just changing nicks, we're good. " << std::endl;
// #endif
  }

  fitView();
}

void
ChatDialog::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
  switch (reason)
  {
  case QSystemTrayIcon::Trigger:
  case QSystemTrayIcon::DoubleClick:
    break;
  case QSystemTrayIcon::MiddleClick:
    // showMessage();
    break;
  default:;
  }
}


void
ChatDialog::messageClicked()
{
  this->showMaximized();
}


void
ChatDialog::createActions()
{
  minimizeAction = new QAction(tr("Mi&nimize"), this);
  connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));

  maximizeAction = new QAction(tr("Ma&ximize"), this);
  connect(maximizeAction, SIGNAL(triggered()), this, SLOT(showMaximized()));

  restoreAction = new QAction(tr("&Restore"), this);
  connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));

  // settingsAction = new QAction(tr("Settings"), this);
  // connect (settingsAction, SIGNAL(triggered()), this, SLOT(buttonPressed()));

  // settingsAction->setMenuRole (QAction::PreferencesRole);

  updateLocalPrefixAction = new QAction(tr("Update local prefix"), this);
  connect (updateLocalPrefixAction, SIGNAL(triggered()), this, SLOT(updateLocalPrefix()));

  quitAction = new QAction(tr("Quit"), this);
  connect(quitAction, SIGNAL(triggered()), this, SLOT(quit()));
}

void
ChatDialog::createTrayIcon()
{
  trayIconMenu = new QMenu(this);
  trayIconMenu->addAction(minimizeAction);
  trayIconMenu->addAction(maximizeAction);
  trayIconMenu->addAction(restoreAction);
  // trayIconMenu->addSeparator();
  // trayIconMenu->addAction(settingsAction);
  trayIconMenu->addSeparator();
  trayIconMenu->addAction(updateLocalPrefixAction);
  trayIconMenu->addSeparator();
  trayIconMenu->addAction(quitAction);

  trayIcon = new QSystemTrayIcon(this);
  trayIcon->setContextMenu(trayIconMenu);

  QIcon icon(":/images/icon_small.png");
  trayIcon->setIcon(icon);
  setWindowIcon(icon);
  trayIcon->setToolTip("ChronoChat System Tray Icon");
  trayIcon->setVisible(true);
}


void
ChatDialog::resizeEvent(QResizeEvent *e)
{
  fitView();
}

void
ChatDialog::showEvent(QShowEvent *e)
{
  fitView();
}

void
ChatDialog::fitView()
{
  boost::recursive_mutex::scoped_lock lock(m_sceneMutex);
  QRectF rect = m_scene->itemsBoundingRect();
  m_scene->setSceneRect(rect);
  ui->treeViewer->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void
ChatDialog::formChatMessage(const QString &text, SyncDemo::ChatMessage &msg) {
  msg.set_from(m_user.getNick().toStdString());
  msg.set_to(m_user.getChatroom().toStdString());
  msg.set_data(text.toUtf8().constData());
  time_t seconds = time(NULL);
  msg.set_timestamp(seconds);
  msg.set_type(SyncDemo::ChatMessage::CHAT);
}

void
ChatDialog::formControlMessage(SyncDemo::ChatMessage &msg, SyncDemo::ChatMessage::ChatMessageType type)
{
  msg.set_from(m_user.getNick().toStdString());
  msg.set_to(m_user.getChatroom().toStdString());
  time_t seconds = time(NULL);
  msg.set_timestamp(seconds);
  msg.set_type(type);
}

void
ChatDialog::updateLocalPrefix()
{
  m_newLocalPrefixReady = false;
  ndn::Ptr<ndn::Interest> interest = ndn::Ptr<ndn::Interest>(new ndn::Interest(ndn::Name("/local/ndn/prefix")));
  interest->setChildSelector(ndn::Interest::CHILD_RIGHT);
  interest->setInterestLifetime(1);

  ndn::Ptr<ndn::Closure> closure = ndn::Ptr<ndn::Closure>(new ndn::Closure(boost::bind(&ChatDialog::onLocalPrefix,
                                                                                       this,
                                                                                       _1),
                                                                           boost::bind(&ChatDialog::onLocalPrefixTimeout,
                                                                                       this,
                                                                                       _1,
                                                                                       _2),
                                                                           boost::bind(&ChatDialog::onLocalPrefix,
                                                                                       this,
                                                                                       _1)));
  
  m_localPrefixHandler->sendInterest(interest, closure);
  while(m_newLocalPrefixReady == false)
    {
#if BOOST_VERSION >= 1050000
      boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
#else
      boost::this_thread::sleep(boost::posix_time::milliseconds(100));
#endif
    }
  _LOG_DEBUG("now the prefix is " << m_newLocalPrefix.toUri());
  _LOG_DEBUG("in use prefix is " << m_user.getOriginPrefix().toStdString());
  QString originPrefix = QString::fromStdString(m_newLocalPrefix.toUri());
    
  if (originPrefix != "" && m_user.getOriginPrefix () != originPrefix)
    emit settingUpdated(m_user.getNick (), m_user.getChatroom (), originPrefix);
}

static std::string chars2("qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM0123456789");

QString
ChatDialog::getRandomString()
{
  std::string randStr;
  boost::random::random_device rng;
  boost::random::uniform_int_distribution<> index_dist(0, chars2.size() - 1);
  for (int i = 0; i < 10; i ++)
  {
    randStr += chars2[index_dist(rng)];
  }
  return randStr.c_str();
}

void
ChatDialog::onLocalPrefix(ndn::Ptr<ndn::Data> data)
{
  string dataString(data->content().buf(), data->content().size());
  QString originPrefix = QString::fromStdString (dataString).trimmed ();
  string trimmedString = originPrefix.toStdString();
  m_newLocalPrefix = ndn::Name(trimmedString);
  m_newLocalPrefixReady = true;
}

void
ChatDialog::onLocalPrefixTimeout(ndn::Ptr<ndn::Closure> closure, ndn::Ptr<ndn::Interest> interest)
{
  m_newLocalPrefix = m_localPrefix;
  m_newLocalPrefixReady = true;
}

void
ChatDialog::changeEvent(QEvent *e)
{
  switch(e->type())
  {
  case QEvent::ActivationChange:
    if (isActiveWindow())
    {
      trayIcon->setIcon(QIcon(":/images/icon_small.png"));
    }
    break;
  default:
    break;
  }
}

void
ChatDialog::appendMessage(const SyncDemo::ChatMessage msg, bool isHistory)
{
  boost::recursive_mutex::scoped_lock lock(m_msgMutex);

  if (msg.type() == SyncDemo::ChatMessage::CHAT)
  {

    if (!msg.has_data())
    {
      return;
    }

    if (msg.from().empty() || msg.data().empty())
    {
      return;
    }

    if (!msg.has_timestamp())
    {
      return;
    }

    // if (m_history.size() == MAX_HISTORY_ENTRY)
    // {
    //   m_history.dequeue();
    // }

    // m_history.enqueue(msg);

    QTextCharFormat nickFormat;
    nickFormat.setForeground(Qt::darkGreen);
    nickFormat.setFontWeight(QFont::Bold);
    nickFormat.setFontUnderline(true);
    nickFormat.setUnderlineColor(Qt::gray);

    QTextCursor cursor(ui->textEdit->textCursor());
    cursor.movePosition(QTextCursor::End);
    QTextTableFormat tableFormat;
    tableFormat.setBorder(0);
    QTextTable *table = cursor.insertTable(1, 2, tableFormat);
    QString from = QString("%1 ").arg(msg.from().c_str());
    QTextTableCell fromCell = table->cellAt(0, 0);
    fromCell.setFormat(nickFormat);
    fromCell.firstCursorPosition().insertText(from);

    time_t timestamp = msg.timestamp();
    printTimeInCell(table, timestamp);

    QTextCursor nextCursor(ui->textEdit->textCursor());
    nextCursor.movePosition(QTextCursor::End);
    table = nextCursor.insertTable(1, 1, tableFormat);
    table->cellAt(0, 0).firstCursorPosition().insertText(QString::fromUtf8(msg.data().c_str()));
    if (!isHistory)
    {
      showMessage(from, QString::fromUtf8(msg.data().c_str()));
    }
  }

  if (msg.type() == SyncDemo::ChatMessage::JOIN || msg.type() == SyncDemo::ChatMessage::LEAVE)
  {
    QTextCharFormat nickFormat;
    nickFormat.setForeground(Qt::gray);
    nickFormat.setFontWeight(QFont::Bold);
    nickFormat.setFontUnderline(true);
    nickFormat.setUnderlineColor(Qt::gray);

    QTextCursor cursor(ui->textEdit->textCursor());
    cursor.movePosition(QTextCursor::End);
    QTextTableFormat tableFormat;
    tableFormat.setBorder(0);
    QTextTable *table = cursor.insertTable(1, 2, tableFormat);
    QString action;
    if (msg.type() == SyncDemo::ChatMessage::JOIN)
    {
      action = "enters room";
    }
    else
    {
      action = "leaves room";
    }

    QString from = QString("%1 %2  ").arg(msg.from().c_str()).arg(action);
    QTextTableCell fromCell = table->cellAt(0, 0);
    fromCell.setFormat(nickFormat);
    fromCell.firstCursorPosition().insertText(from);

    time_t timestamp = msg.timestamp();
    printTimeInCell(table, timestamp);
  }

  QScrollBar *bar = ui->textEdit->verticalScrollBar();
  bar->setValue(bar->maximum());
}

QString
ChatDialog::formatTime(time_t timestamp)
{
  struct tm *tm_time = localtime(&timestamp);
  int hour = tm_time->tm_hour;
  QString amOrPM;
  if (hour > 12)
  {
    hour -= 12;
    amOrPM = "PM";
  }
  else
  {
    amOrPM = "AM";
    if (hour == 0)
    {
      hour = 12;
    }
  }

  char textTime[12];
  sprintf(textTime, "%d:%02d:%02d %s", hour, tm_time->tm_min, tm_time->tm_sec, amOrPM.toStdString().c_str());
  return QString(textTime);
}

void
ChatDialog::printTimeInCell(QTextTable *table, time_t timestamp)
{
  QTextCharFormat timeFormat;
  timeFormat.setForeground(Qt::gray);
  timeFormat.setFontUnderline(true);
  timeFormat.setUnderlineColor(Qt::gray);
  QTextTableCell timeCell = table->cellAt(0, 1);
  timeCell.setFormat(timeFormat);
  timeCell.firstCursorPosition().insertText(formatTime(timestamp));
}

void
ChatDialog::showMessage(QString from, QString data)
{
  if (!isActiveWindow())
  {
    trayIcon->showMessage(QString("Chatroom %1 has a new message").arg(m_user.getChatroom()), QString("<%1>: %2").arg(from).arg(data), QSystemTrayIcon::Information, 20000);
    trayIcon->setIcon(QIcon(":/images/note.png"));
  }
}

void
ChatDialog::sendMsg(SyncDemo::ChatMessage &msg)
{
  // send msg
  size_t size = msg.ByteSize();
  char *buf = new char[size];
  msg.SerializeToArray(buf, size);
  if (!msg.IsInitialized())
  {
    _LOG_DEBUG("Errrrr.. msg was not probally initialized "<<__FILE__ <<":"<<__LINE__<<". what is happening?");
    abort();
  }
  m_sock->publishData(m_user.getPrefix().toStdString(), m_session, buf, size, FRESHNESS);

  delete buf;

  m_lastMsgTime = time(NULL);

  int nextSequence = m_sock->getNextSeq(m_user.getPrefix().toStdString(), m_session);
  Sync::MissingDataInfo mdi = {m_user.getPrefix().toStdString(), Sync::SeqNo(0), Sync::SeqNo(nextSequence - 1)};
  std::vector<Sync::MissingDataInfo> v;
  v.push_back(mdi);
  {
    boost::recursive_mutex::scoped_lock lock(m_sceneMutex);
    m_scene->processUpdate(v, m_sock->getRootDigest().c_str());
    m_scene->msgReceived(m_user.getPrefix(), m_user.getNick());
  }
}

void
ChatDialog::openInviteListDialog()
{
  m_inviteListDialog->setInviteLabel(m_chatroomPrefix.toUri());
  m_inviteListDialog->show();
}

void
ChatDialog::sendInvitationWrapper(QString invitee, bool isIntroducer)
{
  ndn::Name inviteeNamespace(invitee.toUtf8().constData());
  ndn::Ptr<ContactItem> inviteeItem = m_contactManager->getContact(inviteeNamespace);
  sendInvitation(inviteeItem, isIntroducer);
}

void
ChatDialog::closeEvent(QCloseEvent *e)
{
  if (trayIcon->isVisible())
  {
    QMessageBox::information(this, tr("Chronos"),
                             tr("The program will keep running in the "
                                "system tray. To terminate the program"
                                "choose <b>Quit</b> in the context memu"
                                "of the system tray entry."));
    hide();
    e->ignore();
  }
}

void
ChatDialog::quit()
{
  hide();
  emit closeChatDialog(m_chatroomPrefix);
}




#if WAF
#include "chatdialog.moc"
#include "chatdialog.cpp.moc"
#endif
