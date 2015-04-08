/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 *         Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QTimer>
#include "controller.hpp"

#ifndef Q_MOC_RUN
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <ndn-cxx/util/random.hpp>
#include "cryptopp.hpp"
#include "logging.h"
#include "conf.hpp"
#include "endorse-info.hpp"
#endif

INIT_LOGGER("chronochat.Controller");

Q_DECLARE_METATYPE(ndn::Name)
Q_DECLARE_METATYPE(ndn::IdentityCertificate)
Q_DECLARE_METATYPE(chronochat::EndorseInfo)
Q_DECLARE_METATYPE(ndn::Interest)
Q_DECLARE_METATYPE(size_t)
Q_DECLARE_METATYPE(chronochat::ChatroomInfo)
Q_DECLARE_METATYPE(chronochat::Invitation)
Q_DECLARE_METATYPE(std::string)
Q_DECLARE_METATYPE(ndn::Name::Component)

namespace chronochat {

using std::string;

// constructor & destructor
Controller::Controller(QWidget* parent)
  : QDialog(parent)
  , m_localPrefixDetected(false)
  , m_isInConnectionDetection(false)
  , m_settingDialog(new SettingDialog(this))
  , m_startChatDialog(new StartChatDialog(this))
  , m_profileEditor(new ProfileEditor(this))
  , m_invitationDialog(new InvitationDialog(this))
  , m_invitationRequestDialog(new InvitationRequestDialog(this))
  , m_contactPanel(new ContactPanel(this))
  , m_browseContactDialog(new BrowseContactDialog(this))
  , m_addContactPanel(new AddContactPanel(this))
  , m_discoveryPanel(new DiscoveryPanel(this))
{
  qRegisterMetaType<ndn::Name>("ndn.Name");
  qRegisterMetaType<ndn::IdentityCertificate>("ndn.IdentityCertificate");
  qRegisterMetaType<chronochat::EndorseInfo>("chronochat.EndorseInfo");
  qRegisterMetaType<ndn::Interest>("ndn.Interest");
  qRegisterMetaType<size_t>("size_t");
  qRegisterMetaType<chronochat::ChatroomInfo>("chronochat.Chatroom");
  qRegisterMetaType<chronochat::Invitation>("chronochat.Invitation");
  qRegisterMetaType<std::string>("std.string");
  qRegisterMetaType<ndn::Name::Component>("ndn.Component");


  // Connection to ContactManager
  connect(m_backend.getContactManager(), SIGNAL(warning(const QString&)),
          this, SLOT(onWarning(const QString&)));
  connect(this, SIGNAL(refreshBrowseContact()),
          m_backend.getContactManager(), SLOT(onRefreshBrowseContact()));
  connect(m_backend.getContactManager(), SIGNAL(contactInfoFetchFailed(const QString&)),
          this, SLOT(onWarning(const QString&)));

  // Connection to SettingDialog
  connect(this, SIGNAL(identityUpdated(const QString&)),
          m_settingDialog, SLOT(onIdentityUpdated(const QString&)));
  connect(m_settingDialog, SIGNAL(identityUpdated(const QString&)),
          this, SLOT(onIdentityUpdated(const QString&)));
  connect(m_settingDialog, SIGNAL(nickUpdated(const QString&)),
          this, SLOT(onNickUpdated(const QString&)));
  connect(m_settingDialog, SIGNAL(prefixUpdated(const QString&)),
          this, SLOT(onLocalPrefixConfigured(const QString&)));

  // Connection to ProfileEditor
  connect(this, SIGNAL(closeDBModule()),
          m_profileEditor, SLOT(onCloseDBModule()));
  connect(this, SIGNAL(identityUpdated(const QString&)),
          m_profileEditor, SLOT(onIdentityUpdated(const QString&)));
  connect(m_profileEditor, SIGNAL(updateProfile()),
          m_backend.getContactManager(), SLOT(onUpdateProfile()));

  // Connection to StartChatDialog
  connect(m_startChatDialog, SIGNAL(startChatroom(const QString&, bool)),
          this, SLOT(onStartChatroom(const QString&, bool)));

  // Connection to InvitationDialog
  connect(m_invitationDialog, SIGNAL(invitationResponded(const ndn::Name&, bool)),
          &m_backend, SLOT(onInvitationResponded(const ndn::Name&, bool)));

  // Connection to InvitationRequestDialog
  connect(m_invitationRequestDialog, SIGNAL(invitationRequestResponded(const ndn::Name&, bool)),
          &m_backend, SLOT(onInvitationRequestResponded(const ndn::Name&, bool)));

  // Connection to AddContactPanel
  connect(m_addContactPanel, SIGNAL(fetchInfo(const QString&)),
          m_backend.getContactManager(), SLOT(onFetchContactInfo(const QString&)));
  connect(m_addContactPanel, SIGNAL(addContact(const QString&)),
          m_backend.getContactManager(), SLOT(onAddFetchedContact(const QString&)));
  connect(m_backend.getContactManager(),
          SIGNAL(contactEndorseInfoReady(const EndorseInfo&)),
          m_addContactPanel,
          SLOT(onContactEndorseInfoReady(const EndorseInfo&)));

  // Connection to BrowseContactDialog
  connect(m_browseContactDialog, SIGNAL(directAddClicked()),
          this, SLOT(onDirectAdd()));
  connect(m_browseContactDialog, SIGNAL(fetchIdCert(const QString&)),
          m_backend.getContactManager(), SLOT(onFetchIdCert(const QString&)));
  connect(m_browseContactDialog, SIGNAL(addContact(const QString&)),
          m_backend.getContactManager(), SLOT(onAddFetchedContactIdCert(const QString&)));
  connect(m_backend.getContactManager(), SIGNAL(idCertNameListReady(const QStringList&)),
          m_browseContactDialog, SLOT(onIdCertNameListReady(const QStringList&)));
  connect(m_backend.getContactManager(), SIGNAL(nameListReady(const QStringList&)),
          m_browseContactDialog, SLOT(onNameListReady(const QStringList&)));
  connect(m_backend.getContactManager(), SIGNAL(idCertReady(const ndn::IdentityCertificate&)),
          m_browseContactDialog, SLOT(onIdCertReady(const ndn::IdentityCertificate&)));

  // Connection to ContactPanel
  connect(m_contactPanel, SIGNAL(waitForContactList()),
          m_backend.getContactManager(), SLOT(onWaitForContactList()));
  connect(m_contactPanel, SIGNAL(waitForContactInfo(const QString&)),
          m_backend.getContactManager(), SLOT(onWaitForContactInfo(const QString&)));
  connect(m_contactPanel, SIGNAL(removeContact(const QString&)),
          m_backend.getContactManager(), SLOT(onRemoveContact(const QString&)));
  connect(m_contactPanel, SIGNAL(updateAlias(const QString&, const QString&)),
          m_backend.getContactManager(), SLOT(onUpdateAlias(const QString&, const QString&)));
  connect(m_contactPanel, SIGNAL(updateIsIntroducer(const QString&, bool)),
          m_backend.getContactManager(), SLOT(onUpdateIsIntroducer(const QString&, bool)));
  connect(m_contactPanel, SIGNAL(updateEndorseCertificate(const QString&)),
          m_backend.getContactManager(), SLOT(onUpdateEndorseCertificate(const QString&)));
  connect(m_contactPanel, SIGNAL(warning(const QString&)),
          this, SLOT(onWarning(const QString&)));
  connect(this, SIGNAL(closeDBModule()),
          m_contactPanel, SLOT(onCloseDBModule()));
  connect(this, SIGNAL(identityUpdated(const QString&)),
          m_contactPanel, SLOT(onIdentityUpdated(const QString&)));
  connect(m_backend.getContactManager(), SIGNAL(contactAliasListReady(const QStringList&)),
          m_contactPanel, SLOT(onContactAliasListReady(const QStringList&)));
  connect(m_backend.getContactManager(), SIGNAL(contactIdListReady(const QStringList&)),
          m_contactPanel, SLOT(onContactIdListReady(const QStringList&)));
  connect(m_backend.getContactManager(), SIGNAL(contactInfoReady(const QString&, const QString&,
                                                                 const QString&, bool)),
          m_contactPanel, SLOT(onContactInfoReady(const QString&, const QString&,
                                                  const QString&, bool)));

  // Connection to backend thread
  connect(&m_backend, SIGNAL(nfdError()),
          this, SLOT(onNfdError()));
  connect(this, SIGNAL(nfdReconnect()),
          &m_backend, SLOT(onNfdReconnect()));
  connect(this, SIGNAL(shutdownBackend()),
          &m_backend, SLOT(shutdown()));
  connect(this, SIGNAL(updateLocalPrefix()),
          &m_backend, SLOT(onUpdateLocalPrefixAction()));
  connect(this, SIGNAL(identityUpdated(const QString&)),
          &m_backend, SLOT(onIdentityChanged(const QString&)));
  connect(this, SIGNAL(addChatroom(QString)),
          &m_backend, SLOT(addChatroom(QString)));
  connect(this, SIGNAL(removeChatroom(QString)),
          &m_backend, SLOT(removeChatroom(QString)));

  // Thread notifications:
  // on local prefix udpated:
  connect(&m_backend, SIGNAL(localPrefixUpdated(const QString&)),
          this, SLOT(onLocalPrefixUpdated(const QString&)));
  connect(&m_backend, SIGNAL(localPrefixUpdated(const QString&)),
          m_settingDialog, SLOT(onLocalPrefixUpdated(const QString&)));

  // on invitation validated:
  connect(&m_backend, SIGNAL(invitationValidated(QString, QString, ndn::Name)),
          m_invitationDialog, SLOT(onInvitationReceived(QString, QString, ndn::Name)));
  connect(&m_backend, SIGNAL(startChatroom(const QString&, bool)),
          this, SLOT(onStartChatroom(const QString&, bool)));

  // on invitation request received
  connect(&m_backend, SIGNAL(invitationRequestReceived(QString, QString, ndn::Name)),
          m_invitationRequestDialog, SLOT(onInvitationRequestReceived(QString, QString,
                                                                      ndn::Name)));

  // on invitation accepted:
  connect(&m_backend, SIGNAL(startChatroomOnInvitation(chronochat::Invitation, bool)),
          this, SLOT(onStartChatroom2(chronochat::Invitation, bool)));

  m_backend.start();

  initialize();

  m_chatroomDiscoveryBackend
    = new ChatroomDiscoveryBackend(m_localPrefix,
                                   m_identity,
                                   this);

  // connect to chatroom discovery back end
  connect(&m_backend, SIGNAL(localPrefixUpdated(const QString&)),
          m_chatroomDiscoveryBackend, SLOT(updateRoutingPrefix(const QString&)));
  connect(this, SIGNAL(localPrefixConfigured(const QString&)),
          m_chatroomDiscoveryBackend, SLOT(updateRoutingPrefix(const QString&)));
  connect(m_chatroomDiscoveryBackend, SIGNAL(chatroomInfoRequest(std::string, bool)),
          this, SLOT(onChatroomInfoRequest(std::string, bool)));
  connect(this, SIGNAL(respondChatroomInfoRequest(ChatroomInfo, bool)),
          m_chatroomDiscoveryBackend, SLOT(onRespondChatroomInfoRequest(ChatroomInfo, bool)));
  connect(this, SIGNAL(shutdownDiscoveryBackend()),
          m_chatroomDiscoveryBackend, SLOT(shutdown()));
  connect(this, SIGNAL(identityUpdated(const QString&)),
          m_chatroomDiscoveryBackend, SLOT(onIdentityUpdated(const QString&)));
  connect(this, SIGNAL(nfdReconnect()),
          m_chatroomDiscoveryBackend, SLOT(onNfdReconnect()));
  connect(m_chatroomDiscoveryBackend, SIGNAL(nfdError()),
          this, SLOT(onNfdError()));

  // connect chatroom discovery back end with front end
  connect(m_discoveryPanel, SIGNAL(waitForChatroomInfo(const QString&)),
          m_chatroomDiscoveryBackend, SLOT(onWaitForChatroomInfo(const QString&)));
  connect(m_discoveryPanel, SIGNAL(warning(const QString&)),
          this, SLOT(onWarning(const QString&)));
  connect(this, SIGNAL(identityUpdated(const QString&)),
          m_discoveryPanel, SLOT(onIdentityUpdated(const QString&)));
  connect(m_chatroomDiscoveryBackend, SIGNAL(chatroomListReady(const QStringList&)),
          m_discoveryPanel, SLOT(onChatroomListReady(const QStringList&)));
  connect(m_chatroomDiscoveryBackend, SIGNAL(chatroomInfoReady(const ChatroomInfo&, bool)),
          m_discoveryPanel, SLOT(onChatroomInfoReady(const ChatroomInfo&, bool)));
  connect(m_discoveryPanel, SIGNAL(startChatroom(const QString&, bool)),
          this, SLOT(onStartChatroom(const QString&, bool)));
  connect(m_discoveryPanel, SIGNAL(sendInvitationRequest(const QString&, const QString&)),
          &m_backend, SLOT(onSendInvitationRequest(const QString&, const QString&)));
  connect(&m_backend, SIGNAL(invitationRequestResult(const std::string&)),
          m_discoveryPanel, SLOT(onInvitationRequestResult(const std::string&)));

  m_chatroomDiscoveryBackend->start();

  createTrayIcon();

  emit updateLocalPrefix();
}

Controller::~Controller()
{
  saveConf();
}

// public methods


// private methods
string
Controller::getDBName()
{
  string dbName("chronos-");

  std::stringstream ss;
  {
    using namespace CryptoPP;

    SHA256 hash;
    StringSource(m_identity.wireEncode().wire(), m_identity.wireEncode().size(), true,
                 new HashFilter(hash, new HexEncoder(new FileSink(ss), false)));
  }
  dbName.append(ss.str()).append(".db");

  return dbName;
}

void
Controller::openDB()
{
  m_db = QSqlDatabase::addDatabase("QSQLITE");
  QString path = (QDir::home().path());
  path.append(QDir::separator())
    .append(".chronos")
    .append(QDir::separator())
    .append(getDBName().c_str());
  m_db.setDatabaseName(path);

  m_db.open();

  // bool ok = m_db.open();
  // _LOG_DEBUG("DB opened: " << std::boolalpha << ok );
}

void
Controller::initialize()
{
  loadConf();

  openDB();

  emit identityUpdated(QString(m_identity.toUri().c_str()));
}

void
Controller::loadConf()
{
  namespace fs = boost::filesystem;

  fs::path chronosDir = fs::path(getenv("HOME")) / ".chronos";
  fs::create_directories (chronosDir);

  std::ifstream is((chronosDir / "config").c_str ());
  Conf conf;
  Block confBlock;
  try {
    confBlock = ndn::Block::fromStream(is);
    conf.wireDecode(confBlock);
    m_identity.clear();
    m_identity.append(conf.getIdentity());
    if (conf.getNick().length() != 0)
      m_nick = conf.getNick();
    else
      m_nick = m_identity.get(-1).toUri();
  }
  catch (tlv::Error) {
    try {
      ndn::KeyChain keyChain;
      m_identity = keyChain.getDefaultIdentity();
    }
    catch (ndn::KeyChain::Error) {
      m_identity.clear();
      m_identity.append("chronochat-tmp-identity")
        .append(getRandomString());
    }
    m_nick = m_identity.get(-1).toUri();
  }
}

void
Controller::saveConf()
{
  namespace fs = boost::filesystem;

  fs::path chronosDir = fs::path(getenv("HOME")) / ".chronos";
  fs::create_directories (chronosDir);

  std::ofstream os((chronosDir / "config").c_str ());
  Conf conf;
  conf.setIdentity(m_identity);
  if (!m_nick.empty())
    conf.setNick(m_nick);
  Block confWire = conf.wireEncode();
  os.write(reinterpret_cast<const char*>(confWire.wire()), confWire.size());

  os.close();
}

void
Controller::createActions()
{
  m_startChatroom = new QAction(tr("Start new chat"), this);
  connect(m_startChatroom, SIGNAL(triggered()), this, SLOT(onStartChatAction()));

  m_settingsAction = new QAction(tr("Settings"), this);
  connect(m_settingsAction, SIGNAL(triggered()), this, SLOT(onSettingsAction()));

  m_editProfileAction = new QAction(tr("Edit profile"), this);
  connect(m_editProfileAction, SIGNAL(triggered()), this, SLOT(onProfileEditorAction()));

  m_contactListAction = new QAction(tr("Contact List"), this);
  connect(m_contactListAction, SIGNAL(triggered()), this, SLOT(onContactListAction()));

  m_addContactAction = new QAction(tr("Add contact"), this);
  connect(m_addContactAction, SIGNAL(triggered()), this, SLOT(onAddContactAction()));

  m_chatroomDiscoveryAction = new QAction(tr("Chatroom Discovery"), this);
  connect(m_chatroomDiscoveryAction, SIGNAL(triggered()), this, SLOT(onChatroomDiscoveryAction()));

  m_updateLocalPrefixAction = new QAction(tr("Update local prefix"), this);
  connect(m_updateLocalPrefixAction, SIGNAL(triggered()),
          &m_backend, SLOT(onUpdateLocalPrefixAction()));

  m_minimizeAction = new QAction(tr("Mi&nimize"), this);
  connect(m_minimizeAction, SIGNAL(triggered()), this, SLOT(onMinimizeAction()));

  m_quitAction = new QAction(tr("Quit"), this);
  connect(m_quitAction, SIGNAL(triggered()), this, SLOT(onQuitAction()));

}

void
Controller::createTrayIcon()
{
  createActions();

  m_trayIconMenu = new QMenu(this);
  m_trayIconMenu->addAction(m_startChatroom);
  m_trayIconMenu->addAction(m_chatroomDiscoveryAction);

  m_trayIconMenu->addSeparator();
  m_trayIconMenu->addAction(m_settingsAction);
  m_trayIconMenu->addAction(m_editProfileAction);
  m_trayIconMenu->addSeparator();
  m_trayIconMenu->addAction(m_contactListAction);
  m_trayIconMenu->addAction(m_addContactAction);
  m_trayIconMenu->addSeparator();
  m_trayIconMenu->addAction(m_updateLocalPrefixAction);
  m_trayIconMenu->addSeparator();
  m_trayIconMenu->addAction(m_minimizeAction);
  m_closeMenu = m_trayIconMenu->addMenu("Close chatroom");
  m_closeMenu->setEnabled(false);
  m_trayIconMenu->addSeparator();
  m_trayIconMenu->addAction(m_quitAction);

  m_trayIcon = new QSystemTrayIcon(this);
  m_trayIcon->setContextMenu(m_trayIconMenu);

  m_trayIcon->setIcon(QIcon(":/images/icon_small.png"));
  m_trayIcon->setToolTip("ChronoChat System Tray Icon");
  m_trayIcon->setVisible(true);
}

void
Controller::updateMenu()
{
  QMenu* menu = new QMenu(this);
  QMenu* closeMenu = 0;

  menu->addAction(m_startChatroom);
  menu->addAction(m_chatroomDiscoveryAction);

  menu->addSeparator();
  menu->addAction(m_settingsAction);
  menu->addAction(m_editProfileAction);
  menu->addSeparator();
  menu->addAction(m_contactListAction);
  menu->addAction(m_addContactAction);
  menu->addSeparator();
  {
    ChatActionList::const_iterator it = m_chatActionList.begin();
    ChatActionList::const_iterator end = m_chatActionList.end();
    if (it != end) {
      for (; it != end; it++)
        menu->addAction(it->second);
      menu->addSeparator();
    }
  }
  menu->addAction(m_updateLocalPrefixAction);
  menu->addSeparator();
  menu->addAction(m_minimizeAction);
  closeMenu = menu->addMenu("Close chatroom");
  {
    ChatActionList::const_iterator it = m_closeActionList.begin();
    ChatActionList::const_iterator end = m_closeActionList.end();
    if (it == end)
      closeMenu->setEnabled(false);
    else
      for (; it != end; it++)
        closeMenu->addAction(it->second);
  }
  menu->addSeparator();
  menu->addAction(m_quitAction);

  m_trayIcon->setContextMenu(menu);
  delete m_trayIconMenu;
  m_trayIconMenu = menu;
  m_closeMenu = closeMenu;
}

string
Controller::getRandomString()
{
  uint32_t r = ndn::random::generateWord32();
  std::stringstream ss;
  {
    using namespace CryptoPP;
    StringSource(reinterpret_cast<uint8_t*>(&r), 4, true,
                 new HexEncoder(new FileSink(ss), false));

  }
  // for (int i = 0; i < 8; i++)
  //   {
  //     uint32_t t = r & mask;
  //     if (t < 10)
  //       ss << static_cast<char>(t + 0x30);
  //     else
  //       ss << static_cast<char>(t + 0x57);
  //     r = r >> 4;
  //   }

  return ss.str();
}

void
Controller::addChatDialog(const QString& chatroomName, ChatDialog* chatDialog)
{
  m_chatDialogList[chatroomName.toStdString()] = chatDialog;
  connect(chatDialog, SIGNAL(closeChatDialog(const QString&)),
          this, SLOT(onRemoveChatDialog(const QString&)));
  connect(chatDialog, SIGNAL(showChatMessage(const QString&, const QString&, const QString&)),
          this, SLOT(onShowChatMessage(const QString&, const QString&, const QString&)));
  connect(chatDialog, SIGNAL(resetIcon()),
          this, SLOT(onResetIcon()));
  connect(&m_backend, SIGNAL(localPrefixUpdated(const QString&)),
          chatDialog->getBackend(), SLOT(updateRoutingPrefix(const QString&)));
  connect(this, SIGNAL(localPrefixConfigured(const QString&)),
          chatDialog->getBackend(), SLOT(updateRoutingPrefix(const QString&)));
  connect(this, SIGNAL(nfdReconnect()),
          chatDialog->getBackend(), SLOT(onNfdReconnect()));

  // connect chat dialog with discovery backend
  connect(chatDialog->getBackend(), SIGNAL(nfdError()),
          this, SLOT(onNfdError()));
  connect(chatDialog->getBackend(), SIGNAL(newChatroomForDiscovery(ndn::Name::Component)),
          m_chatroomDiscoveryBackend, SLOT(onNewChatroomForDiscovery(ndn::Name::Component)));
  connect(chatDialog->getBackend(), SIGNAL(addInRoster(ndn::Name, ndn::Name::Component)),
          m_chatroomDiscoveryBackend, SLOT(onAddInRoster(ndn::Name, ndn::Name::Component)));
  connect(chatDialog->getBackend(), SIGNAL(eraseInRoster(ndn::Name, ndn::Name::Component)),
          m_chatroomDiscoveryBackend, SLOT(onEraseInRoster(ndn::Name, ndn::Name::Component)));


  QAction* chatAction = new QAction(chatroomName, this);
  m_chatActionList[chatroomName.toStdString()] = chatAction;
  connect(chatAction, SIGNAL(triggered()),
          chatDialog, SLOT(onShow()));

  QAction* closeAction = new QAction(chatroomName, this);
  m_closeActionList[chatroomName.toStdString()] = closeAction;
  connect(closeAction, SIGNAL(triggered()),
          chatDialog, SLOT(shutdown()));

  updateMenu();
}

void
Controller::updateDiscoveryList(const ChatroomInfo& info, bool isAdd)
{
  emit discoverChatroomChanged(info, isAdd);
}

void
Controller::onIdentityUpdated(const QString& identity)
{
  while (!m_chatDialogList.empty()) {
    ChatDialogList::const_iterator it = m_chatDialogList.begin();
    it->second->shutdown();
  }

  emit closeDBModule();

  Name identityName(identity.toStdString());
  m_identity = identityName;

  QTimer::singleShot(500, this, SLOT(onIdentityUpdatedContinued()));
}

void
Controller::onIdentityUpdatedContinued()
{
  QString connection = m_db.connectionName();
  // _LOG_DEBUG("connection name: " << connection.toStdString());
  QSqlDatabase::removeDatabase(connection);
  m_db.close();

  openDB();

  emit identityUpdated(QString(m_identity.toUri().c_str()));
}

void
Controller::onNickUpdated(const QString& nick)
{
  m_nick = nick.toStdString();
}

void
Controller::onLocalPrefixUpdated(const QString& localPrefix)
{
  QString privateLocalPrefix("/private/local");

  if (privateLocalPrefix != localPrefix)
    m_localPrefixDetected = true;
  else
    m_localPrefixDetected = false;

  m_localPrefix = Name(localPrefix.toStdString());
}

void
Controller::onLocalPrefixConfigured(const QString& prefix)
{
  if (!m_localPrefixDetected) {
    m_localPrefix = Name(prefix.toStdString());
    emit localPrefixConfigured(prefix);
  }
}

void
Controller::onStartChatAction()
{
  string chatroom = "chatroom-" + getRandomString();

  m_startChatDialog->setChatroom(chatroom);
  m_startChatDialog->show();
  m_startChatDialog->raise();
}

void
Controller::onSettingsAction()
{
  m_settingDialog->setNick(QString(m_nick.c_str()));
  m_settingDialog->show();
  m_settingDialog->raise();
}

void
Controller::onProfileEditorAction()
{
  m_profileEditor->show();
  m_profileEditor->raise();
}

void
Controller::onAddContactAction()
{
  emit refreshBrowseContact();
  m_browseContactDialog->show();
  m_browseContactDialog->raise();
}

void
Controller::onContactListAction()
{
  m_contactPanel->show();
  m_contactPanel->raise();
}

void
Controller::onChatroomDiscoveryAction()
{
  m_discoveryPanel->show();
  m_discoveryPanel->raise();
}

void
Controller::onDirectAdd()
{
  m_addContactPanel->show();
  m_addContactPanel->raise();
}

void
Controller::onMinimizeAction()
{
  m_settingDialog->hide();
  m_startChatDialog->hide();
  m_profileEditor->hide();
  m_invitationDialog->hide();
  m_addContactPanel->hide();
  m_discoveryPanel->hide();

  ChatDialogList::iterator it = m_chatDialogList.begin();
  ChatDialogList::iterator end = m_chatDialogList.end();
  for (; it != end; it++)
    it->second->hide();
}

void
Controller::onQuitAction()
{
  while (!m_chatDialogList.empty()) {
    ChatDialogList::const_iterator it = m_chatDialogList.begin();
    it->second->shutdown();
  }

  delete m_settingDialog;
  delete m_startChatDialog;
  delete m_profileEditor;
  delete m_invitationDialog;
  delete m_browseContactDialog;
  delete m_addContactPanel;
  delete m_discoveryPanel;
  if (m_chatroomDiscoveryBackend->isRunning()) {
    emit shutdownDiscoveryBackend();
    m_chatroomDiscoveryBackend->wait();
  }
  delete m_chatroomDiscoveryBackend;

  if (m_backend.isRunning()) {
    emit shutdownBackend();
    m_backend.wait();
  }

  if (m_nfdConnectionChecker != nullptr && m_nfdConnectionChecker->isRunning()) {
    emit shutdownNfdChecker();
    m_nfdConnectionChecker->wait();
  }

  QApplication::quit();
}

void
Controller::onStartChatroom(const QString& chatroomName, bool secured)
{
  Name chatroomPrefix;
  chatroomPrefix.append("ndn")
    .append("broadcast")
    .append("ChronoChat")
    .append("Chatroom")
    .append(chatroomName.toStdString());

  // check if the chatroom exists
  if (m_chatDialogList.find(chatroomName.toStdString()) != m_chatDialogList.end()) {
    QMessageBox::information(this, tr("ChronoChat"),
                             tr("You are creating an existing chatroom."
                                "You can check it in the context memu."));
    return;
  }

  // TODO: We should create a chatroom specific key/cert
  //(which should be created in the first half of this method
  //, but let's use the default one for now.
  Name chatPrefix;
  chatPrefix.append(m_identity).append("CHRONOCHAT-CHATDATA").append(chatroomName.toStdString());

  ChatDialog* chatDialog
    = new ChatDialog(chatroomPrefix,
                     chatPrefix,
                     m_localPrefix,
                     chatroomName.toStdString(),
                     m_nick,
                     true,
                     m_identity,
                     this);

  addChatDialog(chatroomName, chatDialog);
  chatDialog->show();

  emit addChatroom(chatroomName);
}

void
Controller::onStartChatroom2(chronochat::Invitation invitation, bool secured)
{
  QString chatroomName = QString::fromStdString(invitation.getChatroom());
  onStartChatroom(chatroomName, secured);

  ChatDialogList::iterator it = m_chatDialogList.find(chatroomName.toStdString());

  BOOST_ASSERT(it != m_chatDialogList.end());
  it->second->addSyncAnchor(invitation);
}


void
Controller::onShowChatMessage(const QString& chatroomName, const QString& from, const QString& data)
{
  m_trayIcon->showMessage(QString("Chatroom %1 has a new message").arg(chatroomName),
                          QString("<%1>: %2").arg(from).arg(data),
                          QSystemTrayIcon::Information, 20000);
  m_trayIcon->setIcon(QIcon(":/images/note.png"));
}

void
Controller::onResetIcon()
{
  m_trayIcon->setIcon(QIcon(":/images/icon_small.png"));
}

void
Controller::onRemoveChatDialog(const QString& chatroomName)
{
  ChatDialogList::iterator it = m_chatDialogList.find(chatroomName.toStdString());
  if (it != m_chatDialogList.end()) {
    ChatDialog* deletedChat = it->second;
    if (deletedChat)
      delete deletedChat;

    m_chatDialogList.erase(it);

    QAction* chatAction = m_chatActionList[chatroomName.toStdString()];
    QAction* closeAction = m_closeActionList[chatroomName.toStdString()];
    if (chatAction)
      delete chatAction;
    if (closeAction)
      delete closeAction;

    m_chatActionList.erase(chatroomName.toStdString());
    m_closeActionList.erase(chatroomName.toStdString());

    updateMenu();
  }
}

void
Controller::onWarning(const QString& msg)
{
  QMessageBox::information(this, tr("ChronoChat"), msg);
}

void
Controller::onNfdError()
{
  if (m_isInConnectionDetection)
    return;
  // begin a thread
  
  m_isInConnectionDetection = true;
  m_nfdConnectionChecker = new NfdConnectionChecker(this);

  connect(m_nfdConnectionChecker, SIGNAL(nfdConnected()),
          this, SLOT(onNfdReconnect()));
  connect(this, SIGNAL(shutdownNfdChecker()),
          m_nfdConnectionChecker, SLOT(shutdown()));

  m_nfdConnectionChecker->start();
  QMessageBox::information(this, tr("ChronoChat"), "Nfd is not running");
}

void
Controller::onNfdReconnect()
{
  if (m_nfdConnectionChecker != nullptr && m_nfdConnectionChecker->isRunning()) {
    m_nfdConnectionChecker->wait();
  }
  delete m_nfdConnectionChecker;
  m_nfdConnectionChecker = nullptr;
  m_isInConnectionDetection = false;
  emit nfdReconnect();
}

void
Controller::onChatroomInfoRequest(std::string chatroomName, bool isManager)
{
  auto it = m_chatDialogList.find(chatroomName);
  if (it != m_chatDialogList.end())
    emit respondChatroomInfoRequest(it->second->getChatroomInfo(), isManager);
}

} // namespace chronochat

#if WAF
#include "controller.moc"
// #include "controller.cpp.moc"
#endif
