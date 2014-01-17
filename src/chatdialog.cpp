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
#include <sync-intro-certificate.h>
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <ndn-cpp/security/signature-sha256-with-rsa.hpp>
#include "logging.h"
#endif

using namespace std;

INIT_LOGGER("ChatDialog");

static const int HELLO_INTERVAL = FRESHNESS * 3 / 4;

Q_DECLARE_METATYPE(std::vector<Sync::MissingDataInfo> )
Q_DECLARE_METATYPE(size_t)

ChatDialog::ChatDialog(ndn::ptr_lib::shared_ptr<ContactManager> contactManager,
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
  , m_invitationPolicy(new SecPolicyChronoChatInvitation(m_chatroomPrefix.get(-1).toEscapedString(), m_defaultIdentity))
  , m_keyChain(new ndn::KeyChain())
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

  startFace();
  m_verifier = ndn::ptr_lib::make_shared<ndn::Verifier>(m_invitationPolicy);
  m_verifier->setFace(m_face);

  ndn::Name certificateName = m_keyChain->getDefaultCertificateNameForIdentity(m_defaultIdentity);
  m_syncPolicy = ndn::ptr_lib::make_shared<SecPolicySync>(m_defaultIdentity, certificateName, m_chatroomPrefix, m_face);

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
  shutdownFace();
}

void
ChatDialog::startFace()
{
  m_face = ndn::ptr_lib::make_shared<ndn::Face>();
  
  connectToDaemon();

  m_running = true;
  m_thread = boost::thread (&ChatDialog::eventLoop, this);  
}

void
ChatDialog::shutdownFace()
{
  {
    boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
    m_running = false;
  }
  
  m_thread.join();
  m_face->shutdown();
}

void
ChatDialog::eventLoop()
{
  while (m_running)
    {
      try{
        m_face->processEvents();
        usleep(1000);
      }catch(std::exception& e){
        _LOG_DEBUG(" " << e.what() );
      }
    }
}

void
ChatDialog::connectToDaemon()
{
  //Hack! transport does not connect to daemon unless an interest is expressed.
  ndn::Name name("/ndn");
  ndn::ptr_lib::shared_ptr<ndn::Interest> interest = ndn::ptr_lib::make_shared<ndn::Interest>(name);
  m_face->expressInterest(*interest, 
                          boost::bind(&ChatDialog::onConnectionData, this, _1, _2),
                          boost::bind(&ChatDialog::onConnectionDataTimeout, this, _1));
}

void
ChatDialog::onConnectionData(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest,
                             const ndn::ptr_lib::shared_ptr<ndn::Data>& data)
{ _LOG_DEBUG("onConnectionData"); }

void
ChatDialog::onConnectionDataTimeout(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest)
{ _LOG_DEBUG("onConnectionDataTimeout"); }

void
ChatDialog::initializeSetting()
{
  m_user.setNick(QString::fromStdString(m_nick));
  m_user.setChatroom(QString::fromStdString(m_chatroomPrefix.get(-1).toEscapedString()));
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
ChatDialog::sendInterest(const ndn::Interest& interest,
                         const ndn::OnVerified& onVerified,
                         const ndn::OnVerifyFailed& onVerifyFailed,
                         const OnEventualTimeout& timeoutNotify,
                         int retry /* = 1 */)
{
  m_face->expressInterest(interest, 
                          boost::bind(&ChatDialog::onTargetData, 
                                      this,
                                      _1,
                                      _2,
                                      onVerified, 
                                      onVerifyFailed),
                          boost::bind(&ChatDialog::onTargetTimeout,
                                      this,
                                      _1,
                                      retry,
                                      onVerified,
                                      onVerifyFailed,
                                      timeoutNotify));
}

void
ChatDialog::onTargetData(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest, 
                         const ndn::ptr_lib::shared_ptr<ndn::Data>& data,
                         const ndn::OnVerified& onVerified,
                         const ndn::OnVerifyFailed& onVerifyFailed)
{
  m_verifier->verifyData(data, onVerified, onVerifyFailed);
}

void
ChatDialog::onTargetTimeout(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest, 
                            int retry,
                            const ndn::OnVerified& onVerified,
                            const ndn::OnVerifyFailed& onVerifyFailed,
                            const OnEventualTimeout& timeoutNotify)
{
  if(retry > 0)
    sendInterest(*interest, onVerified, onVerifyFailed, timeoutNotify, retry-1);
  else
    {
      _LOG_DEBUG("Interest: " << interest->getName().toUri() << " eventually times out!");
      timeoutNotify();
    }
}

void
ChatDialog::sendInvitation(ndn::ptr_lib::shared_ptr<ContactItem> contact, bool isIntroducer)
{
  m_invitationPolicy->addTrustAnchor(contact->getSelfEndorseCertificate());

  ndn::Name certificateName = m_keyChain->getDefaultCertificateNameForIdentity(m_defaultIdentity);

  ndn::Name interestName("/ndn/broadcast/chronos/invitation");
  interestName.append(contact->getNameSpace());
  interestName.append("chatroom");
  interestName.append(m_chatroomPrefix.get(-1));
  interestName.append("inviter-prefix");
  interestName.append(m_localPrefix);
  interestName.append("inviter");
  interestName.append(certificateName);

  string signedUri = interestName.toUri();

  ndn::Signature sig = m_keyChain->sign(reinterpret_cast<const uint8_t*>(signedUri.c_str()), signedUri.size(), certificateName);
  const ndn::Block& sigValue = sig.getValue();

  interestName.append(sigValue);

  //TODO... remove version from invitation interest
  //  interestName.appendVersion();

  ndn::Interest interest(interestName);
  ndn::OnVerified onVerified = boost::bind(&ChatDialog::onInviteReplyVerified,
                                           this,
                                           _1,
                                           contact->getNameSpace(),
                                           isIntroducer);

  ndn::OnVerifyFailed onVerifyFailed = boost::bind(&ChatDialog::onInviteReplyVerifyFailed,
                                                   this,
                                                   _1,
                                                   contact->getNameSpace());

  OnEventualTimeout timeoutNotify = boost::bind(&ChatDialog::onInviteReplyTimeout,
                                                     this,
                                                     contact->getNameSpace());
                                                 

  sendInterest(interest, onVerified, onVerifyFailed, timeoutNotify);
}

void 
ChatDialog::onInviteReplyVerified(const ndn::ptr_lib::shared_ptr<ndn::Data>& data, 
                                  const ndn::Name& identity, 
                                  bool isIntroducer)
{
  string content(reinterpret_cast<const char*>(data->getContent().value()), data->getContent().value_size());
  if(content == string("nack"))
    invitationRejected(identity);
  else
    invitationAccepted(identity, data, content, isIntroducer);
}

void
ChatDialog::onInviteReplyVerifyFailed(const ndn::ptr_lib::shared_ptr<ndn::Data>& data,
                                      const ndn::Name& identity)
{
  _LOG_DEBUG("Reply from " << identity.toUri() << " cannot be verified!");
  QString msg = QString::fromUtf8("Reply from ") + QString::fromStdString(identity.toUri()) + " cannot be verified!";
  emit inivationRejection(msg);
}


void
ChatDialog::onInviteReplyTimeout(const ndn::Name& identity)
{
  _LOG_DEBUG("Your invitation to " << identity.toUri() << " times out!");
  QString msg = QString::fromUtf8("Your invitation to ") + QString::fromStdString(identity.toUri()) + " times out!";
  emit inivationRejection(msg);
}

void
ChatDialog::invitationRejected(const ndn::Name& identity)
{
  _LOG_DEBUG(" " << identity.toUri() << " Rejected your invitation!");
  QString msg = QString::fromStdString(identity.toUri()) + " Rejected your invitation!";
  emit inivationRejection(msg);
}

void
ChatDialog::invitationAccepted(const ndn::Name& identity, ndn::ptr_lib::shared_ptr<ndn::Data> data, const string& inviteePrefix, bool isIntroducer)
{
  _LOG_DEBUG(" " << identity.toUri() << " Accepted your invitation!");
  ndn::SignatureSha256WithRsa sig(data->getSignature());
  const ndn::Name & keyLocatorName = sig.getKeyLocator().getName();
  ndn::ptr_lib::shared_ptr<ndn::IdentityCertificate> dskCertificate = m_invitationPolicy->getValidatedDskCertificate(keyLocatorName);
  m_syncPolicy->addChatDataRule(inviteePrefix, *dskCertificate, isIntroducer);
  publishIntroCert(*dskCertificate, isIntroducer);
}

void
ChatDialog::publishIntroCert(const ndn::IdentityCertificate& dskCertificate, bool isIntroducer)
{
  SyncIntroCertificate syncIntroCertificate(m_chatroomPrefix,
                                            dskCertificate.getPublicKeyName(),
                                            m_keyChain->getDefaultKeyNameForIdentity(m_defaultIdentity),
                                            dskCertificate.getNotBefore(),
                                            dskCertificate.getNotAfter(),
                                            dskCertificate.getPublicKeyInfo(),
                                            (isIntroducer ? SyncIntroCertificate::INTRODUCER : SyncIntroCertificate::PRODUCER));
  ndn::Name certName = m_keyChain->getDefaultCertificateNameForIdentity(m_defaultIdentity);
  _LOG_DEBUG("Publish Intro Certificate: " << syncIntroCertificate.getName());
  m_keyChain->sign(syncIntroCertificate, certName);
  m_face->put(syncIntroCertificate);
}

void
ChatDialog::addTrustAnchor(const EndorseCertificate& selfEndorseCertificate)
{ m_invitationPolicy->addTrustAnchor(selfEndorseCertificate); }

void
ChatDialog::addChatDataRule(const ndn::Name& prefix, 
                            const ndn::IdentityCertificate& identityCertificate,
                            bool isIntroducer)
{ m_syncPolicy->addChatDataRule(prefix, identityCertificate, isIntroducer); }

 

void
ChatDialog::initializeSync()
{
  
  m_sock = new Sync::SyncSocket(m_chatroomPrefix.toUri(),
                                m_syncPolicy,
                                m_face,
                                boost::bind(&ChatDialog::processTreeUpdateWrapper, this, _1, _2),
                                boost::bind(&ChatDialog::processRemoveWrapper, this, _1));
  
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
ChatDialog::processDataWrapper(const ndn::ptr_lib::shared_ptr<ndn::Data>& data)
{
  string name = data->getName().toUri();
  const char* buf = reinterpret_cast<const char*>(data->getContent().value());
  size_t len = data->getContent().value_size();

  char *tempBuf = new char[len];
  memcpy(tempBuf, buf, len);
  emit dataReceived(name.c_str(), tempBuf, len, true, false);
  _LOG_DEBUG("<<< " << name << " fetched");
}

void
ChatDialog::processDataNoShowWrapper(const ndn::ptr_lib::shared_ptr<ndn::Data>& data)
{
  string name = data->getName().toUri();
  const char* buf = reinterpret_cast<const char*>(data->getContent().value());
  size_t len = data->getContent().value_size();

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
                                    m_syncPolicy,
                                    m_face,
                                    bind(&ChatDialog::processTreeUpdateWrapper, this, _1, _2),
                                    bind(&ChatDialog::processRemoveWrapper, this, _1));
      usleep(100000);
      // Sync::CcnxWrapperPtr handle = boost::make_shared<Sync::CcnxWrapper> ();
      // handle->setInterestFilter(m_user.getPrefix().toStdString(), bind(&ChatDialog::respondHistoryRequest, this, _1));
      QTimer::singleShot(600, this, SLOT(sendJoin()));
      m_timer->start(FRESHNESS * 1000);
      disableTreeDisplay();
      QTimer::singleShot(2200, this, SLOT(enableTreeDisplay()));
    }catch(std::exception& e){
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
  ndn::Name interestName("/local/ndn/prefix");
  ndn::Interest interest(interestName);
  interest.setInterestLifetime(1000);

  m_face->expressInterest(interest, 
                          bind(&ChatDialog::onLocalPrefix, this, _1, _2), 
                          bind(&ChatDialog::onLocalPrefixTimeout, this, _1));
  
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


void
ChatDialog::onLocalPrefix(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest, 
                          const ndn::ptr_lib::shared_ptr<ndn::Data>& data)
{
  string dataString(reinterpret_cast<const char*>(data->getContent().value()), data->getContent().value_size());
  QString originPrefix = QString::fromStdString (dataString).trimmed ();
  string trimmedString = originPrefix.toStdString();
  m_newLocalPrefix = ndn::Name(trimmedString);
  m_newLocalPrefixReady = true;
}

void
ChatDialog::onLocalPrefixTimeout(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest)
{
  m_newLocalPrefix = m_localPrefix;
  m_newLocalPrefixReady = true;
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
  ndn::Name inviteeNamespace(invitee.toStdString());
  ndn::ptr_lib::shared_ptr<ContactItem> inviteeItem = m_contactManager->getContact(inviteeNamespace);
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
