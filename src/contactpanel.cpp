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
#include <ndn-cpp-dev/security/validator.hpp>
#include <ndn-cpp-dev/security/signature-sha256-with-rsa.hpp>
#include <boost/filesystem.hpp>
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include "logging.h"
#endif

namespace fs = boost::filesystem;
using namespace ndn;
using namespace chronos;
using namespace std;

INIT_LOGGER("ContactPanel");

Q_DECLARE_METATYPE(ndn::IdentityCertificate)

ContactPanel::ContactPanel(shared_ptr<Face> face,
                           QWidget *parent) 
  : QDialog(parent)
  , ui(new Ui::ContactPanel)
  , m_warningDialog(new WarningDialog)
  , m_contactListModel(new QStringListModel)
  , m_startChatDialog(new StartChatDialog)
  , m_invitationDialog(new InvitationDialog)
  , m_settingDialog(new SettingDialog)
  , m_keyChain(new KeyChain)
  , m_face(face)
  , m_ioService(face->ioService())
  , m_contactManager(new ContactManager(m_face))
{

  qRegisterMetaType<ndn::IdentityCertificate>("IdentityCertificate");

  createAction();

#ifdef WITH_SECURITY
  m_panelValidator = make_shared<chronos::ValidatorPanel>();
  m_invitationValidator = make_shared<chronos::ValidatorInvitation>();
#else
  m_panelValidator = make_shared<ndn::ValidatorNull>();
  m_invitationValidator = make_shared<ndn::ValidatorNull>();
#endif

  connect(&*m_contactManager, SIGNAL(noNdnConnection(const QString&)),
          this, SLOT(showError(const QString&)));
  
  openDB();    

  refreshContactList();

  loadTrustAnchor();

  m_defaultIdentity = m_keyChain->getDefaultIdentity();

  if(m_defaultIdentity.size() == 0)
    showError(QString::fromStdString("certificate of ") + QString::fromStdString(m_defaultIdentity.toUri()) + " is missing!\nHave you installed the certificate?");

  m_keyChain->createIdentity(m_defaultIdentity);

  m_contactManager->setDefaultIdentity(m_defaultIdentity);
  m_nickName = m_defaultIdentity.get(-1).toEscapedString();
  m_settingDialog->setIdentity(m_defaultIdentity.toUri(), m_nickName);
  

  m_profileEditor = new ProfileEditor(m_contactManager);
  m_profileEditor->setCurrentIdentity(m_defaultIdentity);

  m_addContactPanel = new AddContactPanel(m_contactManager);
  m_browseContactDialog = new BrowseContactDialog(m_contactManager);
  m_setAliasDialog = new SetAliasDialog(m_contactManager);
 
  ui->setupUi(this);

  
  m_localPrefix = Name("/private/local");
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

  connect(ui->AddContactButton, SIGNAL(clicked()),
          this, SLOT(openBrowseContactDialog()));

  connect(m_browseContactDialog, SIGNAL(directAddClicked()),
          this, SLOT(openAddContactPanel()));

  connect(this, SIGNAL(refreshCertDirectory()),
          m_browseContactDialog, SLOT(refreshList()));

  connect(ui->DeleteContactButton, SIGNAL(clicked()),
          this, SLOT(removeContactButton()));
   
  connect(ui->settingButton, SIGNAL(clicked()),
          this, SLOT(openSettingDialog()));
  
  connect(ui->chatButton, SIGNAL(clicked()),
          this, SLOT(openStartChatDialog()));
   
  connect(m_addContactPanel, SIGNAL(newContactAdded()),
          this, SLOT(refreshContactList()));
  connect(m_browseContactDialog, SIGNAL(newContactAdded()),
          this, SLOT(refreshContactList()));
  connect(m_setAliasDialog, SIGNAL(aliasChanged()),
          this, SLOT(refreshContactList()));

  connect(m_startChatDialog, SIGNAL(chatroomConfirmed(const QString&)),
          this, SLOT(startChatroom(const QString&)));

  connect(m_invitationDialog, SIGNAL(invitationAccepted(const ndn::Name&)),
          this, SLOT(acceptInvitation(const ndn::Name&)));
  connect(m_invitationDialog, SIGNAL(invitationRejected(const ndn::Name&)),
          this, SLOT(rejectInvitation(const ndn::Name&)));
  
  connect(&*m_contactManager, SIGNAL(contactAdded(const ndn::Name&)),
          this, SLOT(addContactIntoValidator(const ndn::Name&)));
  connect(&*m_contactManager, SIGNAL(contactRemoved(const ndn::Name&)),
          this, SLOT(removeContactFromValidator(const ndn::Name&)));

  connect(m_settingDialog, SIGNAL(identitySet(const QString&, const QString&)),
          this, SLOT(updateDefaultIdentity(const QString&, const QString&)));

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

  delete m_menuAlias;

  map<Name, ChatDialog*>::iterator it = m_chatDialogs.begin();
  for(; it != m_chatDialogs.end(); it++)
    delete it->second;
}

void
ContactPanel::createAction()
{
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
ContactPanel::loadTrustAnchor()
{
#ifdef WITH_SECURITY
  vector<shared_ptr<ContactItem> >::const_iterator it = m_contactList.begin();
  for(; it != m_contactList.end(); it++)
    {
      _LOG_DEBUG("load contact: " << (*it)->getNameSpace().toUri());
      m_invitationValidator->addTrustAnchor((*it)->getSelfEndorseCertificate());
      m_panelValidator->addTrustAnchor((*it)->getSelfEndorseCertificate());
    }
#endif
}

void
ContactPanel::setLocalPrefix(int retry)
{
  Name interestName("/local/ndn/prefix");
  Interest interest(interestName);

  m_face->expressInterest(interest, 
                          bind(&ContactPanel::onLocalPrefix, this, _1, _2), 
                          bind(&ContactPanel::onLocalPrefixTimeout, this, _1, 10));
  
}

void
ContactPanel::onLocalPrefix(const Interest& interest, Data& data)
{
  string originPrefix(reinterpret_cast<const char*>(data.getContent().value()), data.getContent().value_size());
  string prefix = QString::fromStdString (originPrefix).trimmed ().toStdString();
  m_localPrefix = Name(prefix);
}

void
ContactPanel::onLocalPrefixTimeout(const Interest& interest, int retry)
{ 
  if(retry > 0)
    {
      setLocalPrefix(retry - 1);
      return;
    }
  else
    m_localPrefix = Name("/private/local");
}

void
ContactPanel::setInvitationListener()
{
  m_inviteListenPrefix = Name("/ndn/broadcast/chronos/invitation");
  m_inviteListenPrefix.append(m_defaultIdentity);
  _LOG_DEBUG("Listening for invitation on prefix: " << m_inviteListenPrefix.toUri());
  m_invitationListenerId = m_face->setInterestFilter(m_inviteListenPrefix, 
                                                     bind(&ContactPanel::onInvitation, this, _1, _2),
                                                     bind(&ContactPanel::onInvitationRegisterFailed, this, _1, _2));
}

void
ContactPanel::sendInterest(const Interest& interest,
                           const OnDataValidated& onValidated,
                           const OnDataValidationFailed& onValidationFailed,
                           const TimeoutNotify& timeoutNotify,
                           int retry /* = 1 */)
{
  m_face->expressInterest(interest, 
                          bind(&ContactPanel::onTargetData, 
                               this, _1, _2, onValidated, onValidationFailed),
                          bind(&ContactPanel::onTargetTimeout,
                               this, _1, retry, onValidated, onValidationFailed, timeoutNotify));
}

void
ContactPanel::onTargetData(const ndn::Interest& interest, 
                           Data& data,
                           const OnDataValidated& onValidated,
                           const OnDataValidationFailed& onValidationFailed)
{
  m_panelValidator->validate(data, onValidated, onValidationFailed);
}

void
ContactPanel::onTargetTimeout(const ndn::Interest& interest, 
                              int retry,
                              const OnDataValidated& onValidated,
                              const OnDataValidationFailed& onValidationFailed,
                              const TimeoutNotify& timeoutNotify)
{
  if(retry > 0)
    sendInterest(interest, onValidated, onValidationFailed, timeoutNotify, retry-1);
  else
    {
      _LOG_DEBUG("Interest: " << interest.getName() << " eventually times out!");
      timeoutNotify();
    }
}

void
ContactPanel::onInvitationRegisterFailed(const Name& prefix, const string& msg)
{
  showError(QString::fromStdString("Cannot register invitation listening prefix! " + msg));
}

void
ContactPanel::onInvitation(const Name& prefix, const Interest& interest)
{
  _LOG_DEBUG("Receive invitation!" << interest.getName() << " " << prefix);

  OnInterestValidated onValidated = bind(&ContactPanel::onInvitationValidated, this, _1);
  OnInterestValidationFailed onValidationFailed = bind(&ContactPanel::onInvitationValidationFailed, this, _1);
  m_invitationValidator->validate(interest, onValidated, onValidationFailed);
}

void
ContactPanel::popChatInvitation(const Name& interestName)
{
  Invitation invitation(interestName);
  string alias;

  vector<shared_ptr<ContactItem> >::iterator it = m_contactList.begin();
  for(; it != m_contactList.end(); it++)
    if((*it)->getNameSpace() == invitation.getInviteeNameSpace())
      alias = (*it)->getAlias();

  if(it != m_contactList.end())
    return;

  m_invitationDialog->setInvitation(alias, interestName);
  m_invitationDialog->show(); 
}

void
ContactPanel::collectEndorsement()
{
#ifdef WITH_SECURITY
  m_collectStatus = make_shared<vector<bool> >();
  m_collectStatus->assign(m_contactList.size(), false);

  vector<shared_ptr<ContactItem> >::iterator it = m_contactList.begin();
  int count = 0;
  for(; it != m_contactList.end(); it++, count++)
    {
      Name interestName = (*it)->getNameSpace();
      interestName.append("DNS").append(m_defaultIdentity).append("ENDORSEE");
      Interest interest(interestName);
      interest.setInterestLifetime(1000);

      OnDataValidated onValidated = bind(&ContactPanel::onDnsEndorseeValidated, this, _1, count);
      OnDataValidationFailed onValidationFailed = bind(&ContactPanel::onDnsEndorseeValidationFailed, this, _1, count);
      TimeoutNotify timeoutNotify = bind(&ContactPanel::onDnsEndorseeTimeoutNotify, this, count);
  
      sendInterest(interest, onValidated, onValidationFailed, timeoutNotify, 0);
    }
#endif
}

void
ContactPanel::onDnsEndorseeValidated(const shared_ptr<const Data>& data, int count)
{
  Data endorseData;
  endorseData.wireDecode(Block(data->getContent().value(), data->getContent().value_size()));
  EndorseCertificate endorseCertificate(endorseData);

  m_contactManager->getContactStorage()->updateCollectEndorse(endorseCertificate);

  updateCollectStatus(count);
}

void
ContactPanel::onDnsEndorseeTimeoutNotify(int count)
{ updateCollectStatus(count); }

void
ContactPanel::onDnsEndorseeValidationFailed(const shared_ptr<const Data>& data, int count)
{ updateCollectStatus(count); }

void 
ContactPanel::updateCollectStatus(int count)
{
  m_collectStatus->at(count) = true;
  vector<bool>::const_iterator it = m_collectStatus->begin();
  for(; it != m_collectStatus->end(); it++)
    if(*it == false)
      return;

  m_contactManager->publishCollectEndorsedDataInDNS(m_defaultIdentity);
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
  Name defaultIdentity(identity.toStdString());
  Name defaultCertName = m_keyChain->createIdentity(defaultIdentity);

  m_defaultIdentity = defaultIdentity;
  m_profileEditor->setCurrentIdentity(m_defaultIdentity);
  m_nickName = nickName.toStdString();
  
  m_face->unsetInterestFilter(m_invitationListenerId);
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
      vector<shared_ptr<ContactItem> >::iterator contactIt = m_contactList.begin();
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
ContactPanel::addContactIntoValidator(const Name& contactNameSpace)
{
#ifdef WITH_SECURITY
  shared_ptr<ContactItem> contact = m_contactManager->getContact(contactNameSpace);
  if(static_cast<bool>(contact))
    {
      m_panelValidator->addTrustAnchor(contact->getSelfEndorseCertificate());
      m_invitationValidator->addTrustAnchor(contact->getSelfEndorseCertificate());
    }
#endif
}

void
ContactPanel::removeContactFromValidator(const Name& keyName)
{ 
#ifdef WITH_SECURITY
  m_panelValidator->removeTrustAnchor(keyName);
  m_invitationValidator->removeTrustAnchor(keyName);
#endif
}

void
ContactPanel::refreshContactList()
{
  m_contactList.clear();
  m_contactManager->getContactItemList(m_contactList);
  QStringList contactNameList;
  for(int i = 0; i < m_contactList.size(); i++)
    contactNameList << QString::fromStdString(m_contactList[i]->getAlias());

  m_contactListModel->setStringList(contactNameList);
}

void
ContactPanel::showContextMenu(const QPoint& pos)
{
  QMenu menu(ui->ContactList);
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

  m_startChatDialog->setChatroom(chatroom.toUri());
  m_startChatDialog->show();
}

// For inviter
void
ContactPanel::startChatroom(const QString& chatroom)
{
  Name chatroomName(chatroom.toStdString());

  ChatDialog* chatDialog = new ChatDialog(m_contactManager, m_face, chatroomName, m_localPrefix, m_defaultIdentity, m_nickName);
  m_chatDialogs[chatroomName] = chatDialog;
  connect(chatDialog, SIGNAL(closeChatDialog(const ndn::Name&)),
          this, SLOT(removeChatDialog(const ndn::Name&)));
  connect(chatDialog, SIGNAL(noNdnConnection(const QString&)),
          this, SLOT(showError(const QString&)));
  connect(chatDialog, SIGNAL(inivationRejection(const QString&)),
          this, SLOT(showWarning(const QString&)));

  chatDialog->show();
}

// For Invitee
void
ContactPanel::startChatroom2(const Name& invitationInterest)
{
  Invitation invitation(invitationInterest);
  Name chatroomName("/ndn/broadcast/chronos");
  chatroomName.append(invitation.getChatroom());

  ChatDialog* chatDialog = new ChatDialog(m_contactManager, m_face, chatroomName, m_localPrefix, m_defaultIdentity, m_nickName);
  connect(chatDialog, SIGNAL(closeChatDialog(const ndn::Name&)),
          this, SLOT(removeChatDialog(const ndn::Name&)));
  connect(chatDialog, SIGNAL(noNdnConnection(const QString&)),
          this, SLOT(showError(const QString&)));
  connect(chatDialog, SIGNAL(inivationRejection(const QString&)),
          this, SLOT(showWarning(const QString&)));

  Block signatureBlock = invitationInterest.get(-1).blockFromValue();
  Block signatureInfo = invitationInterest.get(-2).blockFromValue();
  Signature signature(signatureInfo, signatureBlock);
  
  SignatureSha256WithRsa sig(signature);
  Name keyLocatorName = sig.getKeyLocator().getName();
  Name inviter = IdentityCertificate::certificateNameToPublicKeyName(keyLocatorName).getPrefix(-1);

#ifdef WITH_SECURITY
  shared_ptr<IdentityCertificate> idCert = m_invitationValidator->getValidatedDskCertificate(keyLocatorName);
  chatDialog->addChatDataRule(invitation.getInviterRoutingPrefix(), *idCert, true);
  chatDialog->publishIntroCert(*idCert, true);
  shared_ptr<ContactItem> inviterItem = m_contactManager->getContact(inviter);
  chatDialog->addTrustAnchor(inviterItem->getSelfEndorseCertificate());
#endif
  
  m_chatDialogs[chatroomName] = chatDialog;

  chatDialog->show();
}

void
ContactPanel::prepareInvitationReply(const Name& invitationInterest, const string& content)
{
  Invitation invitation(invitationInterest);

  Name dataName = invitationInterest;
  dataName.appendVersion();

  Data data(dataName);
  data.setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());
  data.setFreshnessPeriod(1000);

  m_keyChain->signByIdentity(data, invitation.getInviteeNameSpace());

  m_face->put(data);
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
  record.setValue("contact_namespace", QString::fromStdString(m_currentSelectedContact->getNameSpace().toUri()));
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
