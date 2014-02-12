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
#include "invitation.h"

#ifdef WITH_SECURITY
#include <sync-intro-certificate.h>
#endif

#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <ndn-cpp-dev/security/signature-sha256-with-rsa.hpp>
#include "logging.h"
#endif

using namespace std;
using namespace ndn;
using namespace chronos;

INIT_LOGGER("ChatDialog");

static const int HELLO_INTERVAL = FRESHNESS * 3 / 4;

Q_DECLARE_METATYPE(std::vector<Sync::MissingDataInfo> )
Q_DECLARE_METATYPE(ndn::shared_ptr<const ndn::Data> )
Q_DECLARE_METATYPE(size_t)

ChatDialog::ChatDialog(shared_ptr<ContactManager> contactManager,
                       shared_ptr<Face> face,
                       const Name& chatroomPrefix,
		       const Name& localPrefix,
                       const Name& defaultIdentity,
                       const string& nick,
		       QWidget *parent) 
  : QDialog(parent)
  , ui(new Ui::ChatDialog)
  , m_contactManager(contactManager)
  , m_face(face)
  , m_ioService(face->ioService())
  , m_chatroomPrefix(chatroomPrefix)
  , m_localPrefix(localPrefix)
  , m_defaultIdentity(defaultIdentity)
  , m_nick(nick)
  , m_scheduler(*face->ioService())
  , m_keyChain(new KeyChain())
  , m_sock(NULL)
  , m_lastMsgTime(time::now())
  , m_joined(false)
  , m_inviteListDialog(new InviteListDialog(m_contactManager))
{
  qRegisterMetaType<std::vector<Sync::MissingDataInfo> >("std::vector<Sync::MissingDataInfo>");
  qRegisterMetaType<ndn::shared_ptr<const ndn::Data> >("ndn::shared_ptr<const ndn::Data>");
  qRegisterMetaType<size_t>("size_t");

  ui->setupUi(this);

  QString randString = getRandomString();
  m_localChatPrefix = m_localPrefix;
  m_localChatPrefix.append("%F0.").append(m_defaultIdentity);
  m_localChatPrefix.append("chronos").append(m_chatroomPrefix.get(-1)).append(randString.toStdString());

  m_session = static_cast<uint64_t>(time::now());
  m_scene = new DigestTreeScene(m_ioService, this);

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

#ifndef SECURITY
  m_invitationValidator = make_shared<ValidatorNull>();
  m_syncValidator = make_shared<ValidatorNull>();
#else
  m_invitationValidator = make_shared<chronos::ValidatorInvitation>();
  m_syncValidator = ...;
#endif

  connect(ui->inviteButton, SIGNAL(clicked()),
          this, SLOT(openInviteListDialog()));
  connect(m_inviteListDialog, SIGNAL(invitionDetermined(QString, bool)),
          this, SLOT(sendInvitationWrapper(QString, bool)));
  connect(ui->lineEdit, SIGNAL(returnPressed()), 
          this, SLOT(returnPressed()));
  connect(ui->treeButton, SIGNAL(pressed()), 
          this, SLOT(treeButtonPressed()));
  connect(this, SIGNAL(dataReceived(ndn::shared_ptr<const ndn::Data>, bool, bool)), 
          this, SLOT(processData(ndn::shared_ptr<const ndn::Data>, bool, bool)));
  connect(this, SIGNAL(treeUpdated(const std::vector<Sync::MissingDataInfo>)), 
          this, SLOT(processTreeUpdate(const std::vector<Sync::MissingDataInfo>)));
  connect(m_scene, SIGNAL(replot()), 
          this, SLOT(replot()));
  connect(trayIcon, SIGNAL(messageClicked()), 
          this, SLOT(showNormal()));
  connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), 
          this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
  connect(m_scene, SIGNAL(rosterChanged(QStringList)), 
          this, SLOT(updateRosterList(QStringList)));
  connect(this, SIGNAL(triggerHello()),
          this, SLOT(sendHello()));
  connect(this, SIGNAL(triggerJoin()),
          this, SLOT(sendJoin()));
  connect(this, SIGNAL(triggerLeave()),
          this, SLOT(sendLeave()));
  connect(this, SIGNAL(triggerReplot()),
          this, SLOT(replot()));
  connect(this, SIGNAL(triggerEnableTreeDisplay()),
          this, SLOT(enableTreeDisplay()));
  connect(this, SIGNAL(triggerReap()),
          this, SLOT(reap()));
  


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
}

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
ChatDialog::initializeSync()
{
  
  m_sock = new Sync::SyncSocket(m_chatroomPrefix.toUri(),
                                m_syncValidator,
                                m_face,
                                bind(&ChatDialog::processTreeUpdateWrapper, this, _1, _2),
                                bind(&ChatDialog::processRemoveWrapper, this, _1));
  
  usleep(100000);

  m_scheduler.scheduleEvent(time::milliseconds(600), bind(&ChatDialog::sendJoinWrapper, this));

  if(static_cast<bool>(m_replotEventId))
    m_scheduler.cancelEvent(m_replotEventId);
  m_replotEventId = m_scheduler.schedulePeriodicEvent(time::seconds(0), time::milliseconds(FRESHNESS * 1000),
                                                      bind(&ChatDialog::replotWrapper, this));
  disableTreeDisplay();
  m_scheduler.scheduleEvent(time::milliseconds(2200), bind(&ChatDialog::enableTreeDisplayWrapper, this));
}

void
ChatDialog::sendInterest(const ndn::Interest& interest,
                         const OnDataValidated& onValidated,
                         const OnDataValidationFailed& onValidationFailed,
                         const OnEventualTimeout& timeoutNotify,
                         int retry /* = 1 */)
{
  m_face->expressInterest(interest, 
                          bind(&ChatDialog::onTargetData, 
                               this, _1, _2, onValidated, onValidationFailed),
                          bind(&ChatDialog::onTargetTimeout,
                               this, _1, retry, 
                               onValidated, onValidationFailed, timeoutNotify));
}

void
ChatDialog::sendInvitation(shared_ptr<ContactItem> contact, bool isIntroducer)
{
#ifdef WITH_SECURITY
  m_invitationValidator->addTrustAnchor(contact->getSelfEndorseCertificate());
#endif

  Invitation invitation(contact->getNameSpace(),
                               m_chatroomPrefix.get(-1),
                               m_localPrefix);
  ndn::Interest interest(invitation.getUnsignedInterestName());

  m_keyChain->signByIdentity(interest, m_defaultIdentity);

  OnDataValidated onValidated = bind(&ChatDialog::onInviteReplyValidated,
                                     this, _1, contact->getNameSpace(), isIntroducer);

  OnDataValidationFailed onValidationFailed = bind(&ChatDialog::onInviteReplyValidationFailed,
                                                   this, _1, contact->getNameSpace());

  OnEventualTimeout timeoutNotify = bind(&ChatDialog::onInviteReplyTimeout,
                                         this, contact->getNameSpace());
                                                 
  sendInterest(interest, onValidated, onValidationFailed, timeoutNotify);
}

void 
ChatDialog::onInviteReplyValidated(const shared_ptr<const Data>& data, 
                                   const Name& identity, 
                                   bool isIntroducer)
{
  string content(reinterpret_cast<const char*>(data->getContent().value()), data->getContent().value_size());
  if(content == string("nack"))
    invitationRejected(identity);
  else
    invitationAccepted(identity, data, content, isIntroducer);
}

void
ChatDialog::onInviteReplyValidationFailed(const shared_ptr<const Data>& data,
                                          const Name& identity)
{
  QString msg = QString::fromUtf8("Reply from ") + QString::fromStdString(identity.toUri()) + " cannot be verified!";
  emit inivationRejection(msg);
}


void
ChatDialog::onInviteReplyTimeout(const Name& identity)
{
  QString msg = QString::fromUtf8("Your invitation to ") + QString::fromStdString(identity.toUri()) + " times out!";
  emit inivationRejection(msg);
}

void
ChatDialog::invitationRejected(const Name& identity)
{
  QString msg = QString::fromStdString(identity.toUri()) + " Rejected your invitation!";
  emit inivationRejection(msg);
}

void
ChatDialog::invitationAccepted(const Name& identity, 
                               shared_ptr<const Data> data, 
                               const string& inviteePrefix, 
                               bool isIntroducer)
{
  _LOG_DEBUG(" " << identity.toUri() << " Accepted your invitation!");

#ifdef WITH_SECURITY
  SignatureSha256WithRsa sig(data->getSignature());
  const Name & keyLocatorName = sig.getKeyLocator().getName();
  shared_ptr<IdentityCertificate> dskCertificate = m_invitationValidator->getValidatedDskCertificate(keyLocatorName);
  m_syncPolicy->addSyncDataRule(inviteePrefix, *dskCertificate, isIntroducer);
  publishIntroCert(*dskCertificate, isIntroducer);
#endif
}

void
ChatDialog::publishIntroCert(const IdentityCertificate& dskCertificate, bool isIntroducer)
{
#ifdef WITH_SECURITY
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
#endif
}

void
ChatDialog::addChatDataRule(const Name& prefix, 
                            const IdentityCertificate& identityCertificate,
                            bool isIntroducer)
{ 
#ifdef WITH_SECURITY
  m_syncValidator->addSyncDataRule(prefix, identityCertificate, isIntroducer); 
#endif
}

void
ChatDialog::addTrustAnchor(const EndorseCertificate& cert)
{
#ifdef WITH_SECURITY
  m_syncValidator->addTrustAnchor(cert);
#endif
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
ChatDialog::enableTreeDisplayWrapper()
{ emit triggerEnableTreeDisplay(); }

void
ChatDialog::processTreeUpdateWrapper(const vector<Sync::MissingDataInfo>& v, Sync::SyncSocket *sock)
{
  emit treeUpdated(v);
  _LOG_DEBUG("<<< Tree update signal emitted");
}

void
ChatDialog::processRemoveWrapper(string prefix)
{
  _LOG_DEBUG("Sync REMOVE signal received for prefix: " << prefix);
}

void
ChatDialog::processTreeUpdate(const vector<Sync::MissingDataInfo>& v)
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
ChatDialog::processDataWrapper(const shared_ptr<const Data>& data)
{
  emit dataReceived(data, true, false);
  _LOG_DEBUG("<<< " << data->getName() << " fetched");
}

void
ChatDialog::processDataNoShowWrapper(const shared_ptr<const Data>& data)
{ emit dataReceived(data, false, false); }

void
ChatDialog::processData(shared_ptr<const Data> data, bool show, bool isHistory)
{
  SyncDemo::ChatMessage msg;
  bool corrupted = false;
  if (!msg.ParseFromArray(data->getContent().value(), data->getContent().value_size()))
  {
    _LOG_DEBUG("Errrrr.. Can not parse msg with name: " << data->getName() << ". what is happening?");
    // nasty stuff: as a remedy, we'll form some standard msg for inparsable msgs
    msg.set_from("inconnu");
    msg.set_type(SyncDemo::ChatMessage::OTHER);
    corrupted = true;
  }

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
    std::string stdStrName = data->getName().toUri();
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
  m_scheduler.scheduleEvent(time::milliseconds(m_randomizedInterval), bind(&ChatDialog::sendHelloWrapper, this));
}

void
ChatDialog::sendJoinWrapper()
{ emit triggerJoin(); }

void
ChatDialog::sendHello()
{
  int64_t now = time::now();
  int elapsed = (now - m_lastMsgTime) / 1000000000;
  if (elapsed >= m_randomizedInterval / 1000)
  {
    SyncDemo::ChatMessage msg;
    formControlMessage(msg, SyncDemo::ChatMessage::HELLO);
    sendMsg(msg);
    boost::random::random_device rng;
    boost::random::uniform_int_distribution<> uniform(1, FRESHNESS / 5 * 1000);
    m_randomizedInterval = HELLO_INTERVAL * 1000 + uniform(rng);
    m_scheduler.scheduleEvent(time::milliseconds(m_randomizedInterval), bind(&ChatDialog::sendHelloWrapper, this));
  }
  else
  {
    m_scheduler.scheduleEvent(time::milliseconds(m_randomizedInterval - elapsed * 1000), 
                              bind(&ChatDialog::sendHelloWrapper, this));
  }
}

void
ChatDialog::sendHelloWrapper()
{ emit triggerHello(); }

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
ChatDialog::sendLeaveWrapper()
{ emit triggerLeave(); }

void
ChatDialog::replot()
{
  boost::recursive_mutex::scoped_lock lock(m_sceneMutex);
  m_scene->plot(m_sock->getRootDigest().c_str());
  fitView();
}

void
ChatDialog::replotWrapper()
{ emit triggerReplot(); }

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
    m_scheduler.scheduleEvent(time::milliseconds(10000), 
                              bind(&ChatDialog::reapWrapper, this));
  }
}

void
ChatDialog::reapWrapper()
{ emit triggerReap(); }

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
    delete m_sock;
    m_sock = NULL;

    try
    {
      usleep(100000);
      m_sock = new Sync::SyncSocket(m_chatroomPrefix.toUri(),
                                    m_syncValidator,
                                    m_face,
                                    bind(&ChatDialog::processTreeUpdateWrapper, this, _1, _2),
                                    bind(&ChatDialog::processRemoveWrapper, this, _1));
      usleep(100000);
      m_scheduler.scheduleEvent(time::milliseconds(600), bind(&ChatDialog::sendJoinWrapper, this));
      if(static_cast<bool>(m_replotEventId))
        m_scheduler.cancelEvent(m_replotEventId);
      m_replotEventId = m_scheduler.schedulePeriodicEvent(time::seconds(0), time::milliseconds(FRESHNESS * 1000),
                                                          bind(&ChatDialog::replotWrapper, this));
      disableTreeDisplay();
      m_scheduler.scheduleEvent(time::milliseconds(2200), bind(&ChatDialog::enableTreeDisplayWrapper, this));
    }catch(Face::Error& e){
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
  int32_t seconds = static_cast<int32_t>(time::now()/1000000000);
  msg.set_timestamp(seconds);
  msg.set_type(SyncDemo::ChatMessage::CHAT);
}

void
ChatDialog::formControlMessage(SyncDemo::ChatMessage &msg, SyncDemo::ChatMessage::ChatMessageType type)
{
  msg.set_from(m_user.getNick().toStdString());
  msg.set_to(m_user.getChatroom().toStdString());
  int32_t seconds = static_cast<int32_t>(time::now()/1000000000);
  msg.set_timestamp(seconds);
  msg.set_type(type);
}

void
ChatDialog::updateLocalPrefix()
{
  ndn::Name interestName("/local/ndn/prefix");
  ndn::Interest interest(interestName);
  interest.setInterestLifetime(1000);

  m_face->expressInterest(interest, 
                          bind(&ChatDialog::onLocalPrefix, this, _1, _2), 
                          bind(&ChatDialog::onLocalPrefixTimeout, this, _1));  
}


void
ChatDialog::onLocalPrefix(const ndn::Interest& interest, 
                          ndn::Data& data)
{
  string dataString(reinterpret_cast<const char*>(data.getContent().value()), data.getContent().value_size());
  QString originPrefix = QString::fromStdString (dataString).trimmed ();
  string trimmedString = originPrefix.toStdString();
  m_newLocalPrefix = Name(trimmedString);

  _LOG_DEBUG("now the prefix is " << m_newLocalPrefix.toUri());
  _LOG_DEBUG("in use prefix is " << m_user.getOriginPrefix().toStdString());
    
  if (originPrefix != "" && m_user.getOriginPrefix () != originPrefix)
    emit settingUpdated(m_user.getNick (), m_user.getChatroom (), originPrefix);
}

void
ChatDialog::onLocalPrefixTimeout(const ndn::Interest& interest)
{
  m_newLocalPrefix = m_localPrefix;

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

  delete[] buf;

  m_lastMsgTime = time::now();

  uint64_t nextSequence = m_sock->getNextSeq(m_user.getPrefix().toStdString(), m_session);
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
  Name inviteeNamespace(invitee.toStdString());
  shared_ptr<ContactItem> inviteeItem = m_contactManager->getContact(inviteeNamespace);
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
