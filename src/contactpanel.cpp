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
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlField>
#include <QtSql/QSqlError>

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

ContactPanel::ContactPanel(QWidget *parent) 
  : QDialog(parent)
  , ui(new Ui::ContactPanel)
  , m_warningDialog(new WarningDialog)
  , m_contactListModel(new QStringListModel)
  , m_startChatDialog(new StartChatDialog)
  , m_invitationDialog(new InvitationDialog)
  , m_settingDialog(new SettingDialog)
{
  qRegisterMetaType<ndn::security::IdentityCertificate>("IdentityCertificate");
  qRegisterMetaType<ChronosInvitation>("ChronosInvitation");
  
  createAction();

  m_contactManager = Ptr<ContactManager>::Create();

  connect(&*m_contactManager, SIGNAL(noNdnConnection(const QString&)),
          this, SLOT(showError(const QString&)));

  m_contactManager->setWrapper();
  
  openDB();    

  refreshContactList();

  setKeychain();

  m_defaultIdentity = m_keychain->getDefaultIdentity();
  m_contactManager->setDefaultIdentity(m_defaultIdentity);
  m_nickName = m_defaultIdentity.get(-1).toUri();
  m_settingDialog->setIdentity(m_defaultIdentity.toUri(), m_nickName);
  Name defaultCertName = m_keychain->getIdentityManager()->getDefaultCertificateNameByIdentity(m_defaultIdentity);
  if(defaultCertName.size() == 0)
    showError(QString::fromStdString("Corresponding certificate is missing!\nHave you installed the certificate?"));
  

  m_profileEditor = new ProfileEditor(m_contactManager);
  m_profileEditor->setCurrentIdentity(m_defaultIdentity);

  m_addContactPanel = new AddContactPanel(m_contactManager);
  m_browseContactDialog = new BrowseContactDialog(m_contactManager);
  m_setAliasDialog = new SetAliasDialog(m_contactManager);
 
  ui->setupUi(this);

  try{
    m_handler = Ptr<Wrapper>(new Wrapper(m_keychain));  
  }catch(ndn::Error::ndnOperation& e){
    showError(QString::fromStdString("Cannot conect to ndnd!\n Have you started your ndnd?"));
  }
  

  setLocalPrefix();
    
  setInvitationListener();

  collectEndorsement();
  
  ui->ContactList->setModel(m_contactListModel);
  

  connect(ui->ContactList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          this, SLOT(updateSelection(const QItemSelection &, const QItemSelection &)));

  connect(ui->ContactList, SIGNAL(customContextMenuRequested(const QPoint&)),
          this, SLOT(showContextMenu(const QPoint&)));

  connect(ui->EditProfileButton, SIGNAL(clicked()), 
          this, SLOT(openProfileEditor()));

  connect(m_profileEditor, SIGNAL(noKeyOrCert(const QString&)),
          this, SLOT(showWarning(const QString&)));

  // connect(ui->AddContactButton, SIGNAL(clicked()),
  //         this, SLOT(openAddContactPanel()));
  connect(ui->AddContactButton, SIGNAL(clicked()),
          this, SLOT(openBrowseContactDialog()));

  connect(this, SIGNAL(refreshCertDirectory()),
          m_browseContactDialog, SLOT(refreshList()));

  connect(ui->DeleteContactButton, SIGNAL(clicked()),
          this, SLOT(removeContactButton()));
   
  connect(ui->settingButton, SIGNAL(clicked()),
          this, SLOT(openSettingDialog()));
   
  connect(m_browseContactDialog, SIGNAL(newContactAdded()),
          this, SLOT(refreshContactList()));
  connect(m_setAliasDialog, SIGNAL(aliasChanged()),
          this, SLOT(refreshContactList()));

  connect(m_startChatDialog, SIGNAL(chatroomConfirmed(const QString&, const QString&, bool)),
          this, SLOT(startChatroom(const QString&, const QString&, bool)));

  connect(m_invitationDialog, SIGNAL(invitationAccepted(const ChronosInvitation&, const ndn::security::IdentityCertificate&)),
          this, SLOT(acceptInvitation(const ChronosInvitation&, const ndn::security::IdentityCertificate&)));
  connect(m_invitationDialog, SIGNAL(invitationRejected(const ChronosInvitation&)),
          this, SLOT(rejectInvitation(const ChronosInvitation&)));

  connect(m_settingDialog, SIGNAL(identitySet(const QString&, const QString&)),
          this, SLOT(updateDefaultIdentity(const QString&, const QString&)));

  connect(this, SIGNAL(newInvitationReady()),
          this, SLOT(openInvitationDialog()));

  connect(ui->isIntroducer, SIGNAL(stateChanged(int)),
          this, SLOT(isIntroducerChanged(int)));

  connect(ui->addScope, SIGNAL(clicked()),
          this, SLOT(addScopeClicked()));
  connect(ui->deleteScope, SIGNAL(clicked()),
          this, SLOT(deleteScopeClicked()));
  connect(ui->saveButton, SIGNAL(clicked()),
          this, SLOT(saveScopeClicked()));

  connect(ui->endorseButton, SIGNAL(clicked()),
          this, SLOT(endorseButtonClicked()));
}

ContactPanel::~ContactPanel()
{
  delete ui;
  delete m_contactListModel;
  delete m_startChatDialog;
  delete m_invitationDialog;
  delete m_settingDialog;

  delete m_profileEditor;
  delete m_addContactPanel;
  delete m_browseContactDialog;
  delete m_setAliasDialog;

  delete m_trustScopeModel;
  delete m_endorseDataModel;
  delete m_endorseComboBoxDelegate;

  delete m_menuInvite;
  delete m_menuAlias;

  map<Name, ChatDialog*>::iterator it = m_chatDialogs.begin();
  for(; it != m_chatDialogs.end(); it++)
    delete it->second;
}

void
ContactPanel::createAction()
{
  m_menuInvite = new QAction("&Chat", this);
  m_menuAlias = new QAction("&Set Alias", this);
}

void
ContactPanel::openDB()
{
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
  QString path = (QDir::home().path());
  path.append(QDir::separator()).append(".chronos").append(QDir::separator()).append("chronos.db");
  db.setDatabaseName(path);
  bool ok = db.open();
  _LOG_DEBUG("DB opened: " << std::boolalpha << ok );

  m_trustScopeModel = new QSqlTableModel;
  m_endorseDataModel = new QSqlTableModel;
  m_endorseComboBoxDelegate = new EndorseComboBoxDelegate;
}

void 
ContactPanel::setKeychain()
{
  m_panelPolicyManager = Ptr<PanelPolicyManager>::Create();

  vector<Ptr<ContactItem> >::const_iterator it = m_contactList.begin();
  for(; it != m_contactList.end(); it++)
    {
      m_panelPolicyManager->addTrustAnchor((*it)->getSelfEndorseCertificate());
    }

  m_keychain = Ptr<security::Keychain>(new security::Keychain(Ptr<security::IdentityManager>::Create(), 
                                                              m_panelPolicyManager, 
                                                              NULL));
}

void
ContactPanel::setLocalPrefix(int retry)
{
  Ptr<Interest> interest = Ptr<Interest>(new Interest(Name("/local/ndn/prefix")));
  interest->setChildSelector(Interest::CHILD_RIGHT);
  
  Ptr<Closure> closure = Ptr<Closure>(new Closure(boost::bind(&ContactPanel::onLocalPrefixVerified, 
                                                              this,
                                                              _1),
                                                  boost::bind(&ContactPanel::onLocalPrefixTimeout,
                                                              this,
                                                              _1, 
                                                              _2,
                                                              10),
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
  
}

void
ContactPanel::onLocalPrefixTimeout(Ptr<Closure> closure, Ptr<Interest> interest, int retry)
{ 
  if(retry > 0)
    setLocalPrefix(retry - 1);
}

void
ContactPanel::setInvitationListener()
{
  m_inviteListenPrefix = Name("/ndn/broadcast/chronos/invitation");
  m_inviteListenPrefix.append(m_defaultIdentity);
  _LOG_DEBUG("Listening for invitation on prefix: " << m_inviteListenPrefix.toUri());
  m_handler->setInterestFilter (m_inviteListenPrefix, 
                                boost::bind(&ContactPanel::onInvitation, 
                                            this,
                                            _1));
}

void
ContactPanel::onInvitation(Ptr<Interest> interest)
{
  _LOG_DEBUG("Receive invitation!" << interest->getName().toUri());
  
  Ptr<ChronosInvitation> invitation = NULL;
  try{
    invitation = Ptr<ChronosInvitation>(new ChronosInvitation(interest->getName()));
  }catch(exception& e){
    _LOG_ERROR("Exception: " << e.what());
    return;
  }
  
  Name chatroomName("/ndn/broadcast/chronos");
  chatroomName.append(invitation->getChatroom());
  map<Name, ChatDialog*>::iterator it = m_chatDialogs.find(chatroomName);
  if(it != m_chatDialogs.end())
    {
      _LOG_ERROR("Exisiting chatroom!");
      return;
    }

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
ContactPanel::onInvitationCertVerified(Ptr<Data> data, 
                                       Ptr<ChronosInvitation> invitation)
{
  using namespace ndn::security;
  Ptr<IdentityCertificate> certificate = Ptr<IdentityCertificate>(new IdentityCertificate(*data));
  
  if(PolicyManager::verifySignature(invitation->getSignedBlob(), invitation->getSignatureBits(), certificate->getPublicKeyInfo()))
    {
      Name keyName = certificate->getPublicKeyName();
      Name inviterNameSpace = keyName.getPrefix(keyName.size() - 1);
      popChatInvitation(invitation, inviterNameSpace, certificate);
    }
}

void
ContactPanel::onUnverified(Ptr<Data> data)
{}

void
ContactPanel::onTimeout(Ptr<Closure> closure, Ptr<Interest> interest)
{}

void
ContactPanel::popChatInvitation(Ptr<ChronosInvitation> invitation,
                                const Name& inviterNameSpace,
                                Ptr<security::IdentityCertificate> certificate)
{
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

void
ContactPanel::collectEndorsement()
{
  m_collectStatus = Ptr<vector<bool> >::Create();
  m_collectStatus->assign(m_contactList.size(), false);

  vector<Ptr<ContactItem> >::iterator it = m_contactList.begin();
  int count = 0;
  for(; it != m_contactList.end(); it++, count++)
    {
      Name interestName = (*it)->getNameSpace();
      interestName.append("DNS").append(m_defaultIdentity).append("ENDORSEE");
      Ptr<Interest> interest = Ptr<Interest>(new Interest(interestName));
      interest->setChildSelector(Interest::CHILD_RIGHT);
      interest->setInterestLifetime(1);
  
      Ptr<Closure> closure = Ptr<Closure>(new Closure(boost::bind(&ContactPanel::onDnsEndorseeVerified, 
                                                                  this,
                                                                  _1,
                                                                  count),
                                                      boost::bind(&ContactPanel::onDnsEndorseeTimeout,
                                                                  this,
                                                                  _1, 
                                                                  _2,
                                                                  count),
                                                      boost::bind(&ContactPanel::onDnsEndorseeUnverified,
                                                                  this,
                                                                  _1,
                                                                  count)));

      m_handler->sendInterest(interest, closure);
    }
}

void
ContactPanel::onDnsEndorseeVerified(Ptr<Data> data, int count)
{
  Ptr<Blob> contentBlob = Ptr<Blob>(new Blob(data->content().buf(), data->content().size()));
  Ptr<Data> endorseData = Data::decodeFromWire(contentBlob);
  EndorseCertificate endorseCertificate(*endorseData);

  m_contactManager->getContactStorage()->updateCollectEndorse(endorseCertificate);

  updateCollectStatus(count);
}

void
ContactPanel::onDnsEndorseeTimeout(Ptr<Closure> closure, Ptr<Interest> interest, int count)
{ updateCollectStatus(count); }

void
ContactPanel::onDnsEndorseeUnverified(Ptr<Data> data, int count)
{ updateCollectStatus(count); }

void 
ContactPanel::updateCollectStatus(int count)
{
  m_collectStatus->at(count) = true;
  vector<bool>::const_iterator it = m_collectStatus->begin();
  for(; it != m_collectStatus->end(); it++)
    if(*it == false)
      return;

  m_contactManager->publishEndorsedDataInDns(m_defaultIdentity);
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
ContactPanel::showError(const QString& msg){
  QMessageBox::critical(this, tr("Chronos"), msg, QMessageBox::Ok);
  exit(1);
}

void
ContactPanel::showWarning(const QString& msg){
  QMessageBox::information(this, tr("Chronos"), msg);
}

void
ContactPanel::updateSelection(const QItemSelection &selected,
			      const QItemSelection &deselected)
{
  QModelIndexList items = selected.indexes();
  QString text = m_contactListModel->data(items.first(), Qt::DisplayRole).toString();
  string alias = text.toStdString();

  int i = 0;
  for(; i < m_contactList.size(); i++)
    {
      if(alias == m_contactList[i]->getAlias())
        break;
    }
  
  m_currentSelectedContact = m_contactList[i];
  ui->NameData->setText(QString::fromStdString(m_currentSelectedContact->getName()));
  ui->NameSpaceData->setText(QString::fromStdString(m_currentSelectedContact->getNameSpace().toUri()));
  ui->InstitutionData->setText(QString::fromStdString(m_currentSelectedContact->getInstitution()));

  if(m_currentSelectedContact->isIntroducer())
    {
      ui->isIntroducer->setChecked(true);
      ui->addScope->setEnabled(true);
      ui->deleteScope->setEnabled(true);     
      ui->trustScopeList->setEnabled(true);

      string filter("contact_namespace = '");
      filter.append(m_currentSelectedContact->getNameSpace().toUri()).append("'");

      m_trustScopeModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
      m_trustScopeModel->setTable("TrustScope");
      m_trustScopeModel->setFilter(filter.c_str());
      m_trustScopeModel->select();
      m_trustScopeModel->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
      m_trustScopeModel->setHeaderData(1, Qt::Horizontal, QObject::tr("Contact"));
      m_trustScopeModel->setHeaderData(2, Qt::Horizontal, QObject::tr("TrustScope"));

      ui->trustScopeList->setModel(m_trustScopeModel);
      ui->trustScopeList->setColumnHidden(0, true);
      ui->trustScopeList->setColumnHidden(1, true);
      ui->trustScopeList->show();
    }
  else
    {
      ui->isIntroducer->setChecked(false);
      ui->addScope->setEnabled(false);
      ui->deleteScope->setEnabled(false);

      string filter("contact_namespace = '");
      filter.append(m_currentSelectedContact->getNameSpace().toUri()).append("'");

      m_trustScopeModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
      m_trustScopeModel->setTable("TrustScope");
      m_trustScopeModel->setFilter(filter.c_str());
      m_trustScopeModel->select();
      m_trustScopeModel->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
      m_trustScopeModel->setHeaderData(1, Qt::Horizontal, QObject::tr("Contact"));
      m_trustScopeModel->setHeaderData(2, Qt::Horizontal, QObject::tr("TrustScope"));

      ui->trustScopeList->setModel(m_trustScopeModel);
      ui->trustScopeList->setColumnHidden(0, true);
      ui->trustScopeList->setColumnHidden(1, true);
      ui->trustScopeList->show();

      ui->trustScopeList->setEnabled(false);
    }

  string filter("profile_identity = '");
  filter.append(m_currentSelectedContact->getNameSpace().toUri()).append("'");

  m_endorseDataModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
  m_endorseDataModel->setTable("ContactProfile");
  m_endorseDataModel->setFilter(filter.c_str());
  m_endorseDataModel->select();
  
  m_endorseDataModel->setHeaderData(0, Qt::Horizontal, QObject::tr("Identity"));
  m_endorseDataModel->setHeaderData(1, Qt::Horizontal, QObject::tr("Type"));
  m_endorseDataModel->setHeaderData(2, Qt::Horizontal, QObject::tr("Value"));
  m_endorseDataModel->setHeaderData(3, Qt::Horizontal, QObject::tr("Endorse"));

  ui->endorseList->setModel(m_endorseDataModel);
  ui->endorseList->setColumnHidden(0, true);
  ui->endorseList->resizeColumnToContents(1);
  ui->endorseList->resizeColumnToContents(2);
  ui->endorseList->setItemDelegateForColumn(3, m_endorseComboBoxDelegate);
  ui->endorseList->show();
}

void
ContactPanel::updateDefaultIdentity(const QString& identity, const QString& nickName)
{ 
  // _LOG_DEBUG(identity.toStdString());
  Name defaultIdentity = Name(identity.toStdString());
  Name defaultKeyName = m_keychain->getIdentityManager()->getPublicStorage()->getDefaultKeyNameForIdentity(defaultIdentity);
  if(defaultKeyName.size() == 0)
    {
      showWarning(QString::fromStdString("Corresponding key is missing!\nHave you created the key?"));
      return;
    }
  Name defaultCertName = m_keychain->getIdentityManager()->getPublicStorage()->getDefaultCertificateNameForKey(defaultKeyName);
  if(defaultCertName.size() == 0)
    {
      showWarning(QString::fromStdString("Corresponding certificate is missing!\nHave you installed the certificate?"));
      return;
    }
  m_defaultIdentity = defaultIdentity;
  m_profileEditor->setCurrentIdentity(m_defaultIdentity);
  m_nickName = nickName.toStdString();
  m_handler->clearInterestFilter(m_inviteListenPrefix);
  m_contactManager->setDefaultIdentity(m_defaultIdentity);
  setInvitationListener();
  collectEndorsement();
}

void
ContactPanel::openProfileEditor()
{ m_profileEditor->show(); }

void
ContactPanel::openAddContactPanel()
{ m_addContactPanel->show(); }

void
ContactPanel::openBrowseContactDialog()
{ 
  m_browseContactDialog->show(); 
  emit refreshCertDirectory();
}

void
ContactPanel::removeContactButton()
{
  QItemSelectionModel* selectionModel = ui->ContactList->selectionModel();
  QModelIndexList selectedList = selectionModel->selectedIndexes();
  QModelIndexList::iterator it = selectedList.begin();
  for(; it != selectedList.end(); it++)
    {
      string alias =  m_contactListModel->data(*it, Qt::DisplayRole).toString().toStdString();
      vector<Ptr<ContactItem> >::iterator contactIt = m_contactList.begin();
      for(; contactIt != m_contactList.end(); contactIt++)
        {
          if((*contactIt)->getAlias() == alias)
            {
              m_contactManager->getContactStorage()->removeContact((*contactIt)->getNameSpace());
              m_contactList.erase(contactIt);
              break;
            }
        }
    }
  refreshContactList();
}

void
ContactPanel::openInvitationDialog()
{ m_invitationDialog->show(); }

void
ContactPanel::refreshContactList()
{
  m_contactList = m_contactManager->getContactItemList();
  QStringList contactNameList;
  for(int i = 0; i < m_contactList.size(); i++)
    contactNameList << QString::fromStdString(m_contactList[i]->getAlias());

  m_contactListModel->setStringList(contactNameList);
}

void
ContactPanel::showContextMenu(const QPoint& pos)
{
  QMenu menu(ui->ContactList);
  menu.addAction(m_menuInvite);
  connect(m_menuInvite, SIGNAL(triggered()),
          this, SLOT(openStartChatDialog()));
  menu.addSeparator();
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
  m_settingDialog->setIdentity(m_defaultIdentity.toUri(), m_nickName);
  m_settingDialog->show();
}

void
ContactPanel::openStartChatDialog()
{
  Name chatroom("/ndn/broadcast/chronos");
  chatroom.append(string("chatroom-") + getRandomString());

  m_startChatDialog->setInvitee(m_currentSelectedContact->getNameSpace().toUri(), chatroom.toUri());
  m_startChatDialog->show();
}

// For inviter
void
ContactPanel::startChatroom(const QString& chatroom, const QString& invitee, bool isIntroducer)
{
  Name chatroomName(chatroom.toUtf8().constData());

  Name inviteeNamespace(invitee.toStdString());
  Ptr<ContactItem> inviteeItem = m_contactManager->getContact(inviteeNamespace);

  ChatDialog* chatDialog = new ChatDialog(m_contactManager, chatroomName, m_localPrefix, m_defaultIdentity, m_nickName);
  m_chatDialogs.insert(pair <Name, ChatDialog*> (chatroomName, chatDialog));

  connect(chatDialog, SIGNAL(closeChatDialog(const ndn::Name&)),
          this, SLOT(removeChatDialog(const ndn::Name&)));
  connect(chatDialog, SIGNAL(noNdnConnection(const QString&)),
          this, SLOT(showError(const QString&)));

  // send invitation
  chatDialog->sendInvitation(inviteeItem, isIntroducer); 
  
  chatDialog->show();
}

// For Invitee
void
ContactPanel::startChatroom2(const ChronosInvitation& invitation, 
                             const security::IdentityCertificate& identityCertificate)
{
  Ptr<ContactItem> inviterItem = m_contactManager->getContact(invitation.getInviterNameSpace());

  Name chatroomName("/ndn/broadcast/chronos");
  chatroomName.append(invitation.getChatroom());

  ChatDialog* chatDialog = new ChatDialog(m_contactManager, chatroomName, m_localPrefix, m_defaultIdentity, m_nickName, true);

  connect(chatDialog, SIGNAL(closeChatDialog(const ndn::Name&)),
          this, SLOT(removeChatDialog(const ndn::Name&)));
  connect(chatDialog, SIGNAL(noNdnConnection(const QString&)),
          this, SLOT(showError(const QString&)));

  chatDialog->addChatDataRule(invitation.getInviterPrefix(), identityCertificate, true);
  chatDialog->publishIntroCert(identityCertificate, true);

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
  startChatroom2(invitation, identityCertificate);
}

void
ContactPanel::rejectInvitation(const ChronosInvitation& invitation)
{
  string empty("nack");
  m_handler->publishDataByIdentity (invitation.getInterestName(), empty);
}

void
ContactPanel::isIntroducerChanged(int state)
{
  if(state == Qt::Checked)
    {
      ui->addScope->setEnabled(true);
      ui->deleteScope->setEnabled(true);
      ui->trustScopeList->setEnabled(true);
      
      string filter("contact_namespace = '");
      filter.append(m_currentSelectedContact->getNameSpace().toUri()).append("'");

      m_trustScopeModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
      m_trustScopeModel->setTable("TrustScope");
      m_trustScopeModel->setFilter(filter.c_str());
      m_trustScopeModel->select();
      m_trustScopeModel->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
      m_trustScopeModel->setHeaderData(1, Qt::Horizontal, QObject::tr("Contact"));
      m_trustScopeModel->setHeaderData(2, Qt::Horizontal, QObject::tr("TrustScope"));

      ui->trustScopeList->setModel(m_trustScopeModel);
      ui->trustScopeList->setColumnHidden(0, true);
      ui->trustScopeList->setColumnHidden(1, true);
      ui->trustScopeList->show();

      m_currentSelectedContact->setIsIntroducer(true);
    }
  else
    {
      ui->addScope->setEnabled(false);
      ui->deleteScope->setEnabled(false);

      string filter("contact_namespace = '");
      filter.append(m_currentSelectedContact->getNameSpace().toUri()).append("'");

      m_trustScopeModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
      m_trustScopeModel->setTable("TrustScope");
      m_trustScopeModel->setFilter(filter.c_str());
      m_trustScopeModel->select();
      m_trustScopeModel->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
      m_trustScopeModel->setHeaderData(1, Qt::Horizontal, QObject::tr("Contact"));
      m_trustScopeModel->setHeaderData(2, Qt::Horizontal, QObject::tr("TrustScope"));

      ui->trustScopeList->setModel(m_trustScopeModel);
      ui->trustScopeList->setColumnHidden(0, true);
      ui->trustScopeList->setColumnHidden(1, true);
      ui->trustScopeList->show();

      ui->trustScopeList->setEnabled(false);

      m_currentSelectedContact->setIsIntroducer(false);
    }
  m_contactManager->getContactStorage()->updateIsIntroducer(m_currentSelectedContact->getNameSpace(), m_currentSelectedContact->isIntroducer());
}

void
ContactPanel::addScopeClicked()
{
  int rowCount = m_trustScopeModel->rowCount();
  QSqlRecord record;
  QSqlField identityField("contact_namespace", QVariant::String);
  record.append(identityField);
  record.setValue("contact_namespace", QString(m_currentSelectedContact->getNameSpace().toUri().c_str()));
  m_trustScopeModel->insertRow(rowCount);
  m_trustScopeModel->setRecord(rowCount, record);
}

void
ContactPanel::deleteScopeClicked()
{
  QItemSelectionModel* selectionModel = ui->trustScopeList->selectionModel();
  QModelIndexList indexList = selectionModel->selectedIndexes();

  int i = indexList.size() - 1;  
  for(; i >= 0; i--)
    m_trustScopeModel->removeRow(indexList[i].row());
    
  m_trustScopeModel->submitAll();
}

void
ContactPanel::saveScopeClicked()
{ m_trustScopeModel->submitAll(); }

void
ContactPanel::endorseButtonClicked()
{
  m_endorseDataModel->submitAll();
  m_contactManager->updateEndorseCertificate(m_currentSelectedContact->getNameSpace(), m_defaultIdentity);
}

void
ContactPanel::removeChatDialog(const ndn::Name& chatroomName)
{
  map<Name, ChatDialog*>::iterator it = m_chatDialogs.find(chatroomName);

  ChatDialog* deletedChat = NULL;
  if(it != m_chatDialogs.end())
    {
      deletedChat = it->second;
      m_chatDialogs.erase(it);      
    }
  if (deletedChat != NULL)
    delete deletedChat;
}

#if WAF
#include "contactpanel.moc"
#include "contactpanel.cpp.moc"
#endif
