/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "contactpanel.h"
#include "ui_contactpanel.h"


#include <QStringList>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QDir>

#ifndef Q_MOC_RUN
#include <ndn.cxx/security/keychain.h>
#include <ndn.cxx/security/identity/identity-manager.h>
#include <ndn.cxx/common.h>
#include <boost/filesystem.hpp>
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include "panel-policy-manager.h"
#include "logging.h"
#include "exception.h"
#endif

namespace fs = boost::filesystem;
using namespace ndn;

using namespace std;

INIT_LOGGER("ContactPanel");

Q_DECLARE_METATYPE(ndn::security::IdentityCertificate)
Q_DECLARE_METATYPE(ChronosInvitation)

ContactPanel::ContactPanel(Ptr<ContactManager> contactManager, QWidget *parent) 
  : QDialog(parent)
  , ui(new Ui::ContactPanel)
  , m_contactListModel(new QStringListModel)
  , m_startChatDialog(new StartChatDialog)
  , m_invitationDialog(new InvitationDialog)
  , m_settingDialog(new SettingDialog)
  , m_menuInvite(new QAction("&Chat", this))
  , m_menuAlias(new QAction("&Set Alias", this))
{
  qRegisterMetaType<ndn::security::IdentityCertificate>("IdentityCertificate");
  qRegisterMetaType<ChronosInvitation>("ChronosInvitation");
  
  openDB();    

  m_contactManager = contactManager;
  m_profileEditor = new ProfileEditor(m_contactManager);
  m_addContactPanel = new AddContactPanel(contactManager);
  m_setAliasDialog = new SetAliasDialog(contactManager);
 
  ui->setupUi(this);
  refreshContactList();

  setKeychain();
  m_handler = Ptr<Wrapper>(new Wrapper(m_keychain));
   
  setLocalPrefix();
    
  // Set Identity, TODO: through user interface
  m_defaultIdentity = m_keychain->getDefaultIdentity();
  m_settingDialog->setIdentity(m_defaultIdentity.toUri());
  setInvitationListener();
  
  ui->ContactList->setModel(m_contactListModel);
  
  QItemSelectionModel* selectionModel = ui->ContactList->selectionModel();

  connect(selectionModel, SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          this, SLOT(updateSelection(const QItemSelection &, const QItemSelection &)));

  connect(ui->ContactList, SIGNAL(customContextMenuRequested(const QPoint&)),
          this, SLOT(showContextMenu(const QPoint&)));

  connect(ui->EditProfileButton, SIGNAL(clicked()), 
          this, SLOT(openProfileEditor()));

  connect(ui->AddContactButton, SIGNAL(clicked()),
          this, SLOT(openAddContactPanel()));
   
  connect(ui->settingButton, SIGNAL(clicked()),
          this, SLOT(openSettingDialog()));
   
  connect(m_addContactPanel, SIGNAL(newContactAdded()),
          this, SLOT(refreshContactList()));

  connect(m_setAliasDialog, SIGNAL(aliasChanged()),
          this, SLOT(refreshContactList()));

  connect(m_startChatDialog, SIGNAL(chatroomConfirmed(const QString&, const QString&, bool)),
          this, SLOT(startChatroom(const QString&, const QString&, bool)));

  connect(m_invitationDialog, SIGNAL(invitationAccepted(const ChronosInvitation&, const ndn::security::IdentityCertificate&)),
          this, SLOT(acceptInvitation(const ChronosInvitation&, const ndn::security::IdentityCertificate&)));
  connect(m_invitationDialog, SIGNAL(invitationRejected(const ChronosInvitation&)),
          this, SLOT(rejectInvitation(const ChronosInvitation&)));

  connect(m_settingDialog, SIGNAL(identitySet(const QString&)),
          this, SLOT(updateDefaultIdentity(const QString&)));

  connect(this, SIGNAL(newInvitationReady()),
          this, SLOT(openInvitationDialog()));

  connect(ui->isIntroducer, SIGNAL(stateChanged(int)),
          this, SLOT(isIntroducerChanged(int)));
}

ContactPanel::~ContactPanel()
{
  delete ui;
  delete m_contactListModel;
  delete m_profileEditor;
  delete m_addContactPanel;
  if(NULL != m_currentContactTrustScopeListModel)
    delete m_currentContactTrustScopeListModel;

  delete m_menuInvite;

  map<Name, ChatDialog*>::iterator it = m_chatDialogs.begin();
  for(; it != m_chatDialogs.end(); it++)
    delete it->second;
}

void
ContactPanel::openDB()
{
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
  QString path = (QDir::home().path());
  path.append(QDir::separator()).append(".chronos").append(QDir::separator()).append("chronos.db");
  db.setDatabaseName(path);
  bool ok = db.open();
  _LOG_DEBUG("db opened: " << std::boolalpha << ok );
}

void 
ContactPanel::setKeychain()
{
  m_panelPolicyManager = Ptr<PanelPolicyManager>::Create();
  // Ptr<security::EncryptionManager> encryptionManager = Ptr<security::EncryptionManager>(new security::BasicEncryptionManager(privateStorage, "/tmp/encryption.db"));

  vector<Ptr<ContactItem> >::const_iterator it = m_contactList.begin();
  for(; it != m_contactList.end(); it++)
      m_panelPolicyManager->addTrustAnchor((*it)->getSelfEndorseCertificate());

  m_keychain = Ptr<security::Keychain>(new security::Keychain(Ptr<security::IdentityManager>::Create(), 
                                                              m_panelPolicyManager, 
                                                              NULL));
}

void
ContactPanel::setLocalPrefix()
{
  Ptr<Interest> interest = Ptr<Interest>(new Interest(Name("/local/ndn/prefix")));
  interest->setChildSelector(Interest::CHILD_RIGHT);
  
  Ptr<Closure> closure = Ptr<Closure>(new Closure(boost::bind(&ContactPanel::onLocalPrefixVerified, 
                                                              this,
                                                              _1),
                                                  boost::bind(&ContactPanel::onLocalPrefixTimeout,
                                                              this,
                                                              _1, 
                                                              _2),
                                                  boost::bind(&ContactPanel::onLocalPrefixVerified,
                                                              this,
                                                              _1)));

  m_handler->sendInterest(interest, closure);
}

void
ContactPanel::onLocalPrefixVerified(Ptr<Data> data)
{
  string originPrefix(data->content().buf(), data->content().size());
  string prefix = QString::fromStdString (originPrefix).trimmed ().toUtf8().constData();
  string randomSuffix = getRandomString();
  m_localPrefix = Name(prefix);
  m_localPrefix.append(randomSuffix);
}

void
ContactPanel::onLocalPrefixTimeout(Ptr<Closure> closure, Ptr<Interest> interest)
{ 
  string randomSuffix = getRandomString();
  m_localPrefix = Name("/private/local"); 
  m_localPrefix.append(randomSuffix);
}

void
ContactPanel::onUnverified(Ptr<Data> data)
{}

void
ContactPanel::onTimeout(Ptr<Closure> closure, Ptr<Interest> interest)
{}

void
ContactPanel::onInvitationCertVerified(Ptr<Data> data, 
                                       Ptr<ChronosInvitation> invitation)
{
  Ptr<security::IdentityCertificate> certificate = Ptr<security::IdentityCertificate>(new security::IdentityCertificate(*data));
  
  if(security::PolicyManager::verifySignature(invitation->getSignedBlob(), invitation->getSignatureBits(), certificate->getPublicKeyInfo()))
    {
      Name keyName = certificate->getPublicKeyName();
      Name inviterNameSpace = keyName.getSubName(0, keyName.size() - 1);
      popChatInvitation(invitation, inviterNameSpace, certificate);
    }
}

void
ContactPanel::popChatInvitation(Ptr<ChronosInvitation> invitation,
                                const Name& inviterNameSpace,
                                Ptr<security::IdentityCertificate> certificate)
{
  string chatroom = invitation->getChatroom().get(0).toUri();
  string inviter = inviterNameSpace.toUri();

  string alias;
  vector<Ptr<ContactItem> >::iterator it = m_contactList.begin();
  for(; it != m_contactList.end(); it++)
    if((*it)->getNameSpace() == inviterNameSpace)
      alias = (*it)->getAlias();

  if(it != m_contactList.end())
    return;

  m_invitationDialog->setInvitation(alias, invitation, certificate);
  emit newInvitationReady();
}

static std::string chars("qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM0123456789");

string
ContactPanel::getRandomString()
{
  string randStr;
  boost::random::random_device rng;
  boost::random::uniform_int_distribution<> index_dist(0, chars.size() - 1);
  for (int i = 0; i < 10; i ++)
  {
    randStr += chars[index_dist(rng)];
  }
  return randStr;
}


void
ContactPanel::setInvitationListener()
{
  Name prefix("/ndn/broadcast/chronos/invitation");
  prefix.append(m_defaultIdentity);
  _LOG_DEBUG("prefix: " << prefix.toUri());
  m_inviteListenPrefix = prefix;
  m_handler->setInterestFilter (prefix, 
                                boost::bind(&ContactPanel::onInvitation, 
                                            this,
                                            _1));

}

void
ContactPanel::onInvitation(Ptr<Interest> interest)
{
  _LOG_DEBUG("receive interest!" << interest->getName().toUri());
  const Name& interestName = interest->getName();

  Ptr<ChronosInvitation> invitation = Ptr<ChronosInvitation>(new ChronosInvitation(interestName));

  Ptr<Interest> newInterest = Ptr<Interest>(new Interest(invitation->getInviterCertificateName()));
  Ptr<Closure> closure = Ptr<Closure>(new Closure(boost::bind(&ContactPanel::onInvitationCertVerified, 
                                                              this,
                                                              _1,
                                                              invitation),
                                                  boost::bind(&ContactPanel::onTimeout,
                                                              this,
                                                              _1,
                                                              _2),
                                                  boost::bind(&ContactPanel::onUnverified,
                                                              this,
                                                              _1)));
  m_handler->sendInterest(newInterest, closure);
}

void
ContactPanel::updateSelection(const QItemSelection &selected,
			      const QItemSelection &deselected)
{
  QModelIndexList items = selected.indexes();
  QString text = m_contactListModel->data(items.first(), Qt::DisplayRole).toString();
  string alias = text.toUtf8().constData();

  int i = 0;
  for(; i < m_contactList.size(); i++)
    {
      if(alias == m_contactList[i]->getAlias())
        break;
    }
  
  m_currentSelectedContact = m_contactList[i];
  QString name = QString::fromUtf8(m_currentSelectedContact->getName().c_str());
  QString institution = QString::fromUtf8(m_currentSelectedContact->getInstitution().c_str());
  QString nameSpace = QString::fromUtf8(m_currentSelectedContact->getNameSpace().toUri().c_str());
  ui->NameData->setText(name);
  ui->NameSpaceData->setText(nameSpace);
  ui->InstitutionData->setText(institution);

  // m_currentSelectedContactAlias = alias;
  // m_currentSelectedContactNamespace = m_contactList[i]->getNameSpace().toUri();
  // _LOG_DEBUG("current Alias: " << m_currentSelectedContact);

  if(m_currentSelectedContact->isIntroducer())
    {
      ui->isIntroducer->setChecked(true);
      ui->addScope->setEnabled(true);
      ui->deleteScope->setEnabled(true);
      m_currentContactTrustScopeListModel = new QStringListModel;
    }
  else
    {
      ui->isIntroducer->setChecked(false);
      ui->addScope->setEnabled(false);
      ui->deleteScope->setEnabled(false);
      delete m_currentContactTrustScopeListModel;
    }
}

void
ContactPanel::updateDefaultIdentity(const QString& identity)
{ 
  m_defaultIdentity = Name(identity.toUtf8().constData()); 
  Name prefix("/ndn/broadcast/chronos/invitation");
  prefix.append(m_defaultIdentity);

  _LOG_DEBUG("reset invite listen prefix: " << prefix.toUri());
  
  m_handler->clearInterestFilter(m_inviteListenPrefix);
  m_handler->setInterestFilter(prefix,
                               boost::bind(&ContactPanel::onInvitation, 
                                           this,
                                           _1));
  m_inviteListenPrefix = prefix;
}

void
ContactPanel::openProfileEditor()
{ m_profileEditor->show(); }

void
ContactPanel::openAddContactPanel()
{ m_addContactPanel->show(); }

void
ContactPanel::openInvitationDialog()
{ m_invitationDialog->show(); }

void
ContactPanel::refreshContactList()
{
  m_contactList = m_contactManager->getContactItemList();
  QStringList contactNameList;
  for(int i = 0; i < m_contactList.size(); i++)
    contactNameList << QString::fromUtf8(m_contactList[i]->getAlias().c_str());

  m_contactListModel->setStringList(contactNameList);
}

void
ContactPanel::showContextMenu(const QPoint& pos)
{
  QMenu menu(ui->ContactList);
  menu.addAction(m_menuInvite);
  connect(m_menuInvite, SIGNAL(triggered()),
          this, SLOT(openStartChatDialog()));
  menu.addAction(m_menuAlias);
  connect(m_menuAlias, SIGNAL(triggered()),
          this, SLOT(openSetAliasDialog()));
  menu.exec(ui->ContactList->mapToGlobal(pos));

}

void
ContactPanel::openSetAliasDialog()
{
  m_setAliasDialog->setTargetIdentity(m_currentSelectedContact->getNameSpace().toUri());
  m_setAliasDialog->show();
}

void
ContactPanel::openSettingDialog()
{
  m_settingDialog->setIdentity(m_defaultIdentity.toUri());
  m_settingDialog->show();
}

void
ContactPanel::openStartChatDialog()
{
  // TimeInterval ti = time::NowUnixTimestamp();
  // ostringstream oss;
  // oss << ti.total_seconds();

  Name chatroom("/ndn/broadcast/chronos");
  chatroom.append(string("chatroom-") + getRandomString());

  m_startChatDialog->setInvitee(m_currentSelectedContact->getNameSpace().toUri(), chatroom.toUri());
  m_startChatDialog->show();
}

// For inviter
void
ContactPanel::startChatroom(const QString& chatroom, const QString& invitee, bool isIntroducer)
{
  _LOG_DEBUG("room: " << chatroom.toUtf8().constData());
  _LOG_DEBUG("invitee: " << invitee.toUtf8().constData());
  _LOG_DEBUG("introducer: " << std::boolalpha << isIntroducer);

  Name chatroomName(chatroom.toUtf8().constData());
  
  ChatDialog* chatDialog = new ChatDialog(m_contactManager, chatroomName, m_localPrefix, m_defaultIdentity);
  m_chatDialogs.insert(pair <Name, ChatDialog*> (chatroomName, chatDialog));
  
  //TODO: send invitation
  Name inviteeNamespace(invitee.toUtf8().constData());
  Ptr<ContactItem> inviteeItem = m_contactManager->getContact(inviteeNamespace);

  chatDialog->sendInvitation(inviteeItem, isIntroducer); 
  
  chatDialog->show();
}

// For Invitee
void
ContactPanel::startChatroom2(const ChronosInvitation& invitation, 
                             const security::IdentityCertificate& identityCertificate)
{
  _LOG_DEBUG("room: " << invitation.getChatroom().toUri());
  _LOG_DEBUG("inviter: " << invitation.getInviterNameSpace().toUri());

  Name chatroomName("/ndn/broadcast/chronos");
  chatroomName.append(invitation.getChatroom());

  ChatDialog* chatDialog = new ChatDialog(m_contactManager, chatroomName, m_localPrefix, m_defaultIdentity);
  chatDialog->addChatDataRule(invitation.getInviterPrefix(), identityCertificate, true);

  Ptr<ContactItem> inviterItem = m_contactManager->getContact(invitation.getInviterNameSpace());
  chatDialog->addTrustAnchor(inviterItem->getSelfEndorseCertificate());
  
  m_chatDialogs.insert(pair <Name, ChatDialog*> (chatroomName, chatDialog));

  chatDialog->show();
}

void
ContactPanel::acceptInvitation(const ChronosInvitation& invitation, 
                               const security::IdentityCertificate& identityCertificate)
{
  string prefix = m_localPrefix.toUri();

  m_handler->publishDataByIdentity (invitation.getInterestName(), prefix);
  //TODO:: open chat dialog
  _LOG_DEBUG("TO open chat dialog");
  startChatroom2(invitation, identityCertificate);
}

void
ContactPanel::rejectInvitation(const ChronosInvitation& invitation)
{
  string empty;
  m_handler->publishDataByIdentity (invitation.getInterestName(), empty);
}

void
ContactPanel::isIntroducerChanged(int state)
{
  if(state == Qt::Checked)
    {
      ui->addScope->setEnabled(true);
      ui->deleteScope->setEnabled(true);
    }
  else
    {
      ui->addScope->setEnabled(false);
      ui->deleteScope->setEnabled(false);
    }
}

#if WAF
#include "contactpanel.moc"
#include "contactpanel.cpp.moc"
#endif
