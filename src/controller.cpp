/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QTimer>
#include "controller.h"

#ifndef Q_MOC_RUN
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <ndn-cpp-dev/util/random.hpp>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>
#include <cryptopp/filters.h>
#include "config.pb.h"
#include "endorse-info.pb.h"
#include "logging.h"
#endif

INIT_LOGGER("chronos.Controller");

using namespace ndn;

Q_DECLARE_METATYPE(ndn::Name)
Q_DECLARE_METATYPE(ndn::IdentityCertificate)
Q_DECLARE_METATYPE(chronos::EndorseInfo)
Q_DECLARE_METATYPE(ndn::Interest)
Q_DECLARE_METATYPE(size_t)

namespace chronos {

using ndn::shared_ptr;

static const uint8_t ROUTING_PREFIX_SEPARATOR[2] = {0xF0, 0x2E};

// constructor & destructor
Controller::Controller(shared_ptr<Face> face,
		       QWidget* parent)
  : QDialog(parent)
  , m_face(face)
  , m_invitationListenerId(0)
  , m_contactManager(m_face)
  , m_settingDialog(new SettingDialog)
  , m_startChatDialog(new StartChatDialog)
  , m_profileEditor(new ProfileEditor)
  , m_invitationDialog(new InvitationDialog)
  , m_contactPanel(new ContactPanel)
  , m_browseContactDialog(new BrowseContactDialog)
  , m_addContactPanel(new AddContactPanel)
{
  qRegisterMetaType<ndn::Name>("ndn.Name");
  qRegisterMetaType<ndn::IdentityCertificate>("ndn.IdentityCertificate");
  qRegisterMetaType<chronos::EndorseInfo>("chronos.EndorseInfo");
  qRegisterMetaType<ndn::Interest>("ndn.Interest");
  qRegisterMetaType<size_t>("size_t");

  connect(this, SIGNAL(localPrefixUpdated(const QString&)),
          this, SLOT(onLocalPrefixUpdated(const QString&)));
  connect(this, SIGNAL(invitationInterest(const ndn::Name&, const ndn::Interest&, size_t)),
          this, SLOT(onInvitationInterest(const ndn::Name&, const ndn::Interest&, size_t)));

  // Connection to ContactManager
  connect(this, SIGNAL(identityUpdated(const QString&)),
          &m_contactManager, SLOT(onIdentityUpdated(const QString&)));
  connect(&m_contactManager, SIGNAL(warning(const QString&)),
          this, SLOT(onWarning(const QString&)));
  connect(this, SIGNAL(refreshBrowseContact()),
          &m_contactManager, SLOT(onRefreshBrowseContact()));
  connect(&m_contactManager, SIGNAL(contactInfoFetchFailed(const QString&)),
          this, SLOT(onWarning(const QString&)));
  connect(&m_contactManager, SIGNAL(contactIdListReady(const QStringList&)),
          this, SLOT(onContactIdListReady(const QStringList&)));

  // Connection to SettingDialog
  connect(this, SIGNAL(identityUpdated(const QString&)),
          m_settingDialog, SLOT(onIdentityUpdated(const QString&)));
  connect(m_settingDialog, SIGNAL(identityUpdated(const QString&)),
          this, SLOT(onIdentityUpdated(const QString&)));
  connect(m_settingDialog, SIGNAL(nickUpdated(const QString&)),
          this, SLOT(onNickUpdated(const QString&)));

  // Connection to ProfileEditor
  connect(this, SIGNAL(closeDBModule()),
          m_profileEditor, SLOT(onCloseDBModule()));
  connect(this, SIGNAL(identityUpdated(const QString&)),
          m_profileEditor, SLOT(onIdentityUpdated(const QString&)));
  connect(m_profileEditor, SIGNAL(updateProfile()),
          &m_contactManager, SLOT(onUpdateProfile()));

  // Connection to StartChatDialog
  connect(m_startChatDialog, SIGNAL(startChatroom(const QString&, bool)),
          this, SLOT(onStartChatroom(const QString&, bool)));

  // Connection to InvitationDialog
  connect(m_invitationDialog, SIGNAL(invitationResponded(const ndn::Name&, bool)),
          this, SLOT(onInvitationResponded(const ndn::Name&, bool)));

  // Connection to AddContactPanel
  connect(m_addContactPanel, SIGNAL(fetchInfo(const QString&)),
          &m_contactManager, SLOT(onFetchContactInfo(const QString&)));
  connect(m_addContactPanel, SIGNAL(addContact(const QString&)),
          &m_contactManager, SLOT(onAddFetchedContact(const QString&)));
  connect(&m_contactManager, SIGNAL(contactEndorseInfoReady(const chronos::EndorseInfo&)),
          m_addContactPanel, SLOT(onContactEndorseInfoReady(const chronos::EndorseInfo&)));


  // Connection to BrowseContactDialog
  connect(m_browseContactDialog, SIGNAL(directAddClicked()),
          this, SLOT(onDirectAdd()));
  connect(m_browseContactDialog, SIGNAL(fetchIdCert(const QString&)),
          &m_contactManager, SLOT(onFetchIdCert(const QString&)));
  connect(m_browseContactDialog, SIGNAL(addContact(const QString&)),
          &m_contactManager, SLOT(onAddFetchedContactIdCert(const QString&)));
  connect(&m_contactManager, SIGNAL(idCertNameListReady(const QStringList&)),
          m_browseContactDialog, SLOT(onIdCertNameListReady(const QStringList&)));
  connect(&m_contactManager, SIGNAL(nameListReady(const QStringList&)),
          m_browseContactDialog, SLOT(onNameListReady(const QStringList&)));
  connect(&m_contactManager, SIGNAL(idCertReady(const ndn::IdentityCertificate&)),
          m_browseContactDialog, SLOT(onIdCertReady(const ndn::IdentityCertificate&)));

  // Connection to ContactPanel
  connect(m_contactPanel, SIGNAL(waitForContactList()),
          &m_contactManager, SLOT(onWaitForContactList()));
  connect(m_contactPanel, SIGNAL(waitForContactInfo(const QString&)),
          &m_contactManager, SLOT(onWaitForContactInfo(const QString&)));
  connect(m_contactPanel, SIGNAL(removeContact(const QString&)),
          &m_contactManager, SLOT(onRemoveContact(const QString&)));
  connect(m_contactPanel, SIGNAL(updateAlias(const QString&, const QString&)),
          &m_contactManager, SLOT(onUpdateAlias(const QString&, const QString&)));
  connect(m_contactPanel, SIGNAL(updateIsIntroducer(const QString&, bool)),
          &m_contactManager, SLOT(onUpdateIsIntroducer(const QString&, bool)));
  connect(m_contactPanel, SIGNAL(updateEndorseCertificate(const QString&)),
          &m_contactManager, SLOT(onUpdateEndorseCertificate(const QString&)));
  connect(m_contactPanel, SIGNAL(warning(const QString&)),
          this, SLOT(onWarning(const QString&)));
  connect(this, SIGNAL(closeDBModule()),
          m_contactPanel, SLOT(onCloseDBModule()));
  connect(this, SIGNAL(identityUpdated(const QString&)),
          m_contactPanel, SLOT(onIdentityUpdated(const QString&)));
  connect(&m_contactManager, SIGNAL(contactAliasListReady(const QStringList&)),
          m_contactPanel, SLOT(onContactAliasListReady(const QStringList&)));
  connect(&m_contactManager, SIGNAL(contactIdListReady(const QStringList&)),
          m_contactPanel, SLOT(onContactIdListReady(const QStringList&)));
  connect(&m_contactManager, SIGNAL(contactInfoReady(const QString&, const QString&, const QString&, bool)),
          m_contactPanel, SLOT(onContactInfoReady(const QString&, const QString&, const QString&, bool)));

  initialize();

  createTrayIcon();

  onUpdateLocalPrefixAction();
}
  
Controller::~Controller()
{
  saveConf();
}

// public methods


// private methods
std::string
Controller::getDBName()
{
  std::string dbName("chronos-");

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
  path.append(QDir::separator()).append(".chronos").append(QDir::separator()).append(getDBName().c_str());
  m_db.setDatabaseName(path);
  bool ok = m_db.open();

  _LOG_DEBUG("DB opened: " << std::boolalpha << ok );
}

void
Controller::initialize()
{
  loadConf();
  
  m_keyChain.createIdentity(m_identity);

  openDB();

  emit identityUpdated(QString(m_identity.toUri().c_str()));

  setInvitationListener();
}

void
Controller::setInvitationListener()
{
  if(m_invitationListenerId != 0)
    m_face->unsetInterestFilter(m_invitationListenerId);
  
  Name invitationPrefix;
  Name routingPrefix = getInvitationRoutingPrefix();
  size_t offset = 0;
  if(!routingPrefix.isPrefixOf(m_identity))
    {
      invitationPrefix.append(routingPrefix).append(ROUTING_PREFIX_SEPARATOR, 2);
      offset = routingPrefix.size() + 1;
    }
  invitationPrefix.append(m_identity).append("CHRONOCHAT-INVITATION");

  m_invitationListenerId = m_face->setInterestFilter(invitationPrefix, 
                                                     bind(&Controller::onInvitationInterestWrapper, this, _1, _2, offset),
                                                     bind(&Controller::onInvitationRegisterFailed, this, _1, _2));
}

void
Controller::loadConf()
{
  namespace fs = boost::filesystem;

  fs::path chronosDir = fs::path(getenv("HOME")) / ".chronos";
  fs::create_directories (chronosDir);

  std::ifstream is((chronosDir / "config").c_str ());
  ChronoChat::Conf conf;
  if(conf.ParseFromIstream(&is))
    {
      m_identity.clear();
      m_identity.append(conf.identity());
      if(conf.has_nick())
        m_nick = conf.nick();
      else
        m_nick = m_identity.get(-1).toUri();
    }
  else
    {
      m_identity.clear();
      // TODO: change below to system default;
      m_identity.append("chronochat-tmp-identity")
        .append(getRandomString());
      
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
  ChronoChat::Conf conf;
  conf.set_identity(m_identity.toUri());
  if(!m_nick.empty())
    conf.set_nick(m_nick);
  conf.SerializeToOstream(&os);

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

  m_updateLocalPrefixAction = new QAction(tr("Update local prefix"), this);
  connect(m_updateLocalPrefixAction, SIGNAL(triggered()), this, SLOT(onUpdateLocalPrefixAction()));

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
    if(it != end)
      {
        for(; it != end; it++)
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
    if(it == end)
      {
        closeMenu->setEnabled(false);
      }
    else
      {
        for(; it != end; it++)
          closeMenu->addAction(it->second);
      }
  }
  menu->addSeparator();
  menu->addAction(m_quitAction);
  
  m_trayIcon->setContextMenu(menu);
  delete m_trayIconMenu;
  m_trayIconMenu = menu;
  m_closeMenu = closeMenu;
}

void
Controller::onLocalPrefix(const Interest& interest, Data& data)
{
  QString localPrefixStr = QString::fromUtf8
    (reinterpret_cast<const char*>(data.getContent().value()), data.getContent().value_size())
    .trimmed();

  Name localPrefix(localPrefixStr.toStdString());
  if(m_localPrefix.empty() || m_localPrefix != localPrefix)
    emit localPrefixUpdated(localPrefixStr);
}

void
Controller::onLocalPrefixTimeout(const Interest& interest)
{
  QString localPrefixStr("/private/local");

  Name localPrefix(localPrefixStr.toStdString());
  if(m_localPrefix.empty() || m_localPrefix != localPrefix)
    emit localPrefixUpdated(localPrefixStr);
}

void
Controller::onInvitationInterestWrapper(const Name& prefix, const Interest& interest, size_t routingPrefixOffset)
{
  emit invitationInterest(prefix, interest, routingPrefixOffset);
}

void
Controller::onInvitationRegisterFailed(const Name& prefix, const std::string& failInfo)
{
  _LOG_DEBUG("Controller::onInvitationRegisterFailed: " << failInfo);
}

void
Controller::onInvitationValidated(const shared_ptr<const Interest>& interest)
{
  Invitation invitation(interest->getName());
  std::string alias = invitation.getInviterCertificate().getPublicKeyName().getPrefix(-1).toUri(); // Should be obtained via a method of ContactManager.

  m_invitationDialog->setInvitation(alias, invitation.getChatroom(), interest->getName());
  m_invitationDialog->show();
}

void
Controller::onInvitationValidationFailed(const shared_ptr<const Interest>& interest, std::string failureInfo)
{
  _LOG_DEBUG("Invitation: " << interest->getName() << " cannot not be validated due to: " << failureInfo);
}

std::string
Controller::getRandomString()
{
  uint32_t r = random::generateWord32();
  std::stringstream ss;
  {
    using namespace CryptoPP;
    StringSource(reinterpret_cast<uint8_t*>(&r), 4, true,
                 new HexEncoder(new FileSink(ss), false));
    
  }
  // for(int i = 0; i < 8; i++)
  //   {
  //     uint32_t t = r & mask;
  //     if(t < 10)
  //       ss << static_cast<char>(t + 0x30);
  //     else
  //       ss << static_cast<char>(t + 0x57);
  //     r = r >> 4;
  //   }

  return ss.str();
}

ndn::Name
Controller::getInvitationRoutingPrefix()
{
  return Name("/ndn/broadcast");
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
  connect(this, SIGNAL(localPrefixUpdated(const QString&)),
          chatDialog, SLOT(onLocalPrefixUpdated(const QString&)));

  QAction* chatAction = new QAction(chatroomName, this);
  m_chatActionList[chatroomName.toStdString()] = chatAction;
  connect(chatAction, SIGNAL(triggered()), chatDialog, SLOT(raise()));

  QAction* closeAction = new QAction(chatroomName, this);
  m_closeActionList[chatroomName.toStdString()] = closeAction;
  connect(closeAction, SIGNAL(triggered()), chatDialog, SLOT(onClose()));

  updateMenu();
}

// private slots:
void
Controller::onIdentityUpdated(const QString& identity)
{
  Name identityName(identity.toStdString());

  while(!m_chatDialogList.empty())
    {
      ChatDialogList::const_iterator it = m_chatDialogList.begin();
      onRemoveChatDialog(QString::fromStdString(it->first));
    }

  m_identity = identityName;
  m_keyChain.createIdentity(m_identity);
  setInvitationListener();

  emit closeDBModule();
  
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
Controller::onContactIdListReady(const QStringList& list)
{
  ContactList contactList;

  m_contactManager.getContactList(contactList);
  m_validator.cleanTrustAnchor();

  ContactList::const_iterator it  = contactList.begin();
  ContactList::const_iterator end = contactList.end();

  for(; it != end; it++)
    m_validator.addTrustAnchor((*it)->getPublicKeyName(), (*it)->getPublicKey());

}

void
Controller::onNickUpdated(const QString& nick)
{
  m_nick = nick.toStdString();
}

void
Controller::onLocalPrefixUpdated(const QString& localPrefix)
{
  m_localPrefix = Name(localPrefix.toStdString());
}

void
Controller::onStartChatAction()
{
  std::string chatroom = "chatroom-" + getRandomString();

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
Controller::onDirectAdd()
{
  m_addContactPanel->show();
  m_addContactPanel->raise();
}

void
Controller::onUpdateLocalPrefixAction()
{
  // Name interestName();
  Interest interest("/local/ndn/prefix");
  interest.setInterestLifetime(time::milliseconds(1000));
  interest.setMustBeFresh(true);

  m_face->expressInterest(interest, 
                          bind(&Controller::onLocalPrefix, this, _1, _2), 
                          bind(&Controller::onLocalPrefixTimeout, this, _1));  
}

void
Controller::onMinimizeAction()
{
  m_settingDialog->hide();
  m_startChatDialog->hide();
  m_profileEditor->hide();
  m_invitationDialog->hide();
  m_addContactPanel->hide();

  ChatDialogList::iterator it = m_chatDialogList.begin();
  ChatDialogList::iterator end = m_chatDialogList.end();
  for(; it != end; it++)
    it->second->hide();
}

void
Controller::onQuitAction()
{
  while(!m_chatDialogList.empty())
    {
      ChatDialogList::const_iterator it = m_chatDialogList.begin();
      onRemoveChatDialog(QString::fromStdString(it->first));
    }

  if(m_invitationListenerId != 0)
    m_face->unsetInterestFilter(m_invitationListenerId);

  delete m_settingDialog;
  delete m_startChatDialog;
  delete m_profileEditor;
  delete m_invitationDialog;
  delete m_browseContactDialog;
  delete m_addContactPanel;

  m_face->ioService()->stop();

  QApplication::quit();
}

void
Controller::onStartChatroom(const QString& chatroomName, bool secured)
{
  Name chatroomPrefix;
  chatroomPrefix.append("ndn")
    .append("broadcast")
    .append("ChronoChat")
    .append(chatroomName.toStdString());

  // check if the chatroom exists
  if(m_chatDialogList.find(chatroomName.toStdString()) != m_chatDialogList.end())
    {
      QMessageBox::information(this, tr("ChronoChat"),
                               tr("You are creating an existing chatroom."
                                  "You can check it in the context memu."));
      return;
    }

  // TODO: We should create a chatroom specific key/cert (which should be created in the first half of this method, but let's use the default one for now.
  shared_ptr<IdentityCertificate> idCert = m_keyChain.getCertificate(m_keyChain.getDefaultCertificateNameForIdentity(m_identity));
  ChatDialog* chatDialog = new ChatDialog(&m_contactManager, m_face, *idCert, chatroomPrefix, m_localPrefix, m_nick, secured);

  addChatDialog(chatroomName, chatDialog);  
  chatDialog->show();
}

void
Controller::onInvitationResponded(const Name& invitationName, bool accepted)
{
  Data response;
  shared_ptr<IdentityCertificate> chatroomCert;

  // generate reply;
  if(accepted)
    {
      Name responseName = invitationName;
      responseName.append(m_localPrefix.wireEncode());

      response.setName(responseName);

      // We should create a particular certificate for this chatroom, but let's use default one for now.
      chatroomCert = m_keyChain.getCertificate(m_keyChain.getDefaultCertificateNameForIdentity(m_identity));

      response.setContent(chatroomCert->wireEncode());
      response.setFreshnessPeriod(time::milliseconds(1000));
    }
  else
    {
      response.setName(invitationName);
      response.setFreshnessPeriod(time::milliseconds(1000));
    }
  m_keyChain.signByIdentity(response, m_identity);
  
  // Check if we need a wrapper
  Name invitationRoutingPrefix = getInvitationRoutingPrefix();
  if(invitationRoutingPrefix.isPrefixOf(m_identity))
    {
      m_face->put(response);
    }
  else
    {
      Name wrappedName;
      wrappedName.append(invitationRoutingPrefix)
        .append(ROUTING_PREFIX_SEPARATOR, 2)
        .append(response.getName());

      _LOG_DEBUG("onInvitationResponded: prepare reply " << wrappedName);
      
      Data wrappedData(wrappedName);
      wrappedData.setContent(response.wireEncode());
      wrappedData.setFreshnessPeriod(time::milliseconds(1000));

      m_keyChain.signByIdentity(wrappedData, m_identity);
      m_face->put(wrappedData);
    }

  // create chatroom
  if(accepted)
    {
      Invitation invitation(invitationName);
      Name chatroomPrefix;
      chatroomPrefix.append("ndn")
        .append("broadcast")
        .append("ChronoChat")
        .append(invitation.getChatroom());

      //We should create a chatroom specific key/cert (which should be created in the first half of this method, but let's use the default one for now.
      shared_ptr<IdentityCertificate> idCert = m_keyChain.getCertificate(m_keyChain.getDefaultCertificateNameForIdentity(m_identity));
      ChatDialog* chatDialog = new ChatDialog(&m_contactManager, m_face, *idCert, chatroomPrefix, m_localPrefix, m_nick, true);
      chatDialog->addSyncAnchor(invitation);

      addChatDialog(QString::fromStdString(invitation.getChatroom()), chatDialog);
      chatDialog->show();
    }
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

  if(it != m_chatDialogList.end())
    {
      ChatDialog* deletedChat = it->second;
      if(deletedChat)
        delete deletedChat;
      m_chatDialogList.erase(it);

      QAction* chatAction = m_chatActionList[chatroomName.toStdString()];
      QAction* closeAction = m_closeActionList[chatroomName.toStdString()];
      if(chatAction)
        delete chatAction;
      if(closeAction)
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
Controller::onError(const QString& msg) 
{
  QMessageBox::critical(this, tr("ChronoChat"), msg, QMessageBox::Ok);
  exit(1);
}

void
Controller::onInvitationInterest(const Name& prefix, const Interest& interest, size_t routingPrefixOffset)
{
  _LOG_DEBUG("onInvitationInterest: " << interest.getName());
  shared_ptr<Interest> invitationInterest = make_shared<Interest>(boost::cref(interest.getName().getSubName(routingPrefixOffset)));

  // check if the chatroom already exists;
  try
    {
      Invitation invitation(invitationInterest->getName());
      if(m_chatDialogList.find(invitation.getChatroom()) != m_chatDialogList.end())
        return;
    }
  catch(Invitation::Error& e)
    {
      // Cannot parse the invitation;
      return;
    }

  OnInterestValidated onValidated = bind(&Controller::onInvitationValidated, this, _1);
  OnInterestValidationFailed onValidationFailed = bind(&Controller::onInvitationValidationFailed, this, _1, _2);
  m_validator.validate(*invitationInterest, onValidated, onValidationFailed);
}

} // namespace chronos

#if WAF
#include "controller.moc"
#include "controller.cpp.moc"
#endif
