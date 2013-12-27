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
#include <ndn-cpp/security/identity/osx-private-key-storage.hpp>
#include <ndn-cpp/security/identity/basic-identity-storage.hpp>
#include <ndn-cpp/security/signature/sha256-with-rsa-handler.hpp>
#include <boost/filesystem.hpp>
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include "panel-policy-manager.h"
#include "null-ptrs.h"
#include "logging.h"
#include "exception.h"
#endif

namespace fs = boost::filesystem;
using namespace ndn;
using namespace ndn::ptr_lib;

using namespace std;

INIT_LOGGER("ContactPanel");

Q_DECLARE_METATYPE(ndn::IdentityCertificate)
Q_DECLARE_METATYPE(ChronosInvitation)

ContactPanel::ContactPanel(QWidget *parent) 
  : QDialog(parent)
  , ui(new Ui::ContactPanel)
  , m_warningDialog(new WarningDialog)
  , m_contactListModel(new QStringListModel)
  , m_startChatDialog(new StartChatDialog)
  , m_invitationDialog(new InvitationDialog)
  , m_settingDialog(new SettingDialog)
  , m_policyManager(new PanelPolicyManager())
{
  qRegisterMetaType<IdentityCertificate>("IdentityCertificate");
  qRegisterMetaType<ChronosInvitation>("ChronosInvitation");

  startFace();  

  createAction();

  shared_ptr<BasicIdentityStorage> publicStorage = make_shared<BasicIdentityStorage>();
  shared_ptr<OSXPrivateKeyStorage> privateStorage = make_shared<OSXPrivateKeyStorage>();
  m_identityManager = make_shared<IdentityManager>(publicStorage, privateStorage);

  m_contactManager = make_shared<ContactManager>(m_identityManager, m_face, m_transport);

  connect(&*m_contactManager, SIGNAL(noNdnConnection(const QString&)),
          this, SLOT(showError(const QString&)));
  
  openDB();    

  refreshContactList();

  loadTrustAnchor();

  m_defaultIdentity = m_identityManager->getDefaultIdentity();
  if(m_defaultIdentity.size() == 0)
    showError(QString::fromStdString("certificate of ") + QString::fromStdString(m_defaultIdentity.toUri()) + " is missing!\nHave you installed the certificate?");
  Name defaultCertName = m_identityManager->getDefaultCertificateNameForIdentity(m_defaultIdentity);
  if(defaultCertName.size() == 0)
    showError(QString::fromStdString("certificate of ") + QString::fromStdString(m_defaultIdentity.toUri()) + " is missing!\nHave you installed the certificate?");


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
   
  connect(m_addContactPanel, SIGNAL(newContactAdded()),
          this, SLOT(refreshContactList()));
  connect(m_browseContactDialog, SIGNAL(newContactAdded()),
          this, SLOT(refreshContactList()));
  connect(m_setAliasDialog, SIGNAL(aliasChanged()),
          this, SLOT(refreshContactList()));

  connect(m_startChatDialog, SIGNAL(chatroomConfirmed(const QString&, const QString&, bool)),
          this, SLOT(startChatroom(const QString&, const QString&, bool)));

  connect(m_invitationDialog, SIGNAL(invitationAccepted(const ChronosInvitation&, const ndn::IdentityCertificate&)),
          this, SLOT(acceptInvitation(const ChronosInvitation&, const ndn::IdentityCertificate&)));
  connect(m_invitationDialog, SIGNAL(invitationRejected(const ChronosInvitation&)),
          this, SLOT(rejectInvitation(const ChronosInvitation&)));
  
  connect(&*m_contactManager, SIGNAL(contactAdded(const ndn::Name&)),
          this, SLOT(addContactIntoPanelPolicy(const ndn::Name&)));
  connect(&*m_contactManager, SIGNAL(contactRemoved(const ndn::Name&)),
          this, SLOT(removeContactFromPanelPolicy(const ndn::Name&)));

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

  shutdownFace();
}

void
ContactPanel::startFace()
{
  m_transport = make_shared<TcpTransport>();
  m_face = make_shared<Face>(m_transport, make_shared<TcpTransport::ConnectionInfo>("localhost"));
  
  connectToDaemon();

  m_running = true;
  m_thread = boost::thread (&ContactPanel::eventLoop, this);  
}

void
ContactPanel::shutdownFace()
{
  {
    boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
    m_running = false;
  }
  
  m_thread.join();
  m_face->shutdown();
}

void
ContactPanel::eventLoop()
{
  while (m_running)
    {
      try{
        m_face->processEvents();
        usleep(100);
      }catch(std::exception& e){
        _LOG_DEBUG(" " << e.what() );
      }
    }
}

void
ContactPanel::connectToDaemon()
{
  //Hack! transport does not connect to daemon unless an interest is expressed.
  Name name("/ndn");
  ndn::Interest interest(name);
  m_face->expressInterest(interest, 
                          bind(&ContactPanel::onConnectionData, this, _1, _2),
                          bind(&ContactPanel::onConnectionDataTimeout, this, _1));
}

void
ContactPanel::onConnectionData(const shared_ptr<const ndn::Interest>& interest,
                               const shared_ptr<Data>& data)
{ _LOG_DEBUG("onConnectionData"); }

void
ContactPanel::onConnectionDataTimeout(const shared_ptr<const ndn::Interest>& interest)
{ _LOG_DEBUG("onConnectionDataTimeout"); }

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
ContactPanel::loadTrustAnchor()
{
  vector<shared_ptr<ContactItem> >::const_iterator it = m_contactList.begin();
  for(; it != m_contactList.end(); it++)
    {
      _LOG_DEBUG("load contact: " << (*it)->getNameSpace().toUri());
      m_policyManager->addTrustAnchor((*it)->getSelfEndorseCertificate());
    }
}

void
ContactPanel::setLocalPrefix(int retry)
{
  Name interestName("/local/ndn/prefix");
  Interest interest(interestName);
  interest.setChildSelector(ndn_Interest_CHILD_SELECTOR_RIGHT);

  m_face->expressInterest(interest, 
                          bind(&ContactPanel::onLocalPrefix, this, _1, _2), 
                          bind(&ContactPanel::onLocalPrefixTimeout, this, _1, 10));
  
}

void
ContactPanel::onLocalPrefix(const shared_ptr<const Interest>& interest, 
                            const shared_ptr<Data>& data)
{
  string originPrefix((const char*)data->getContent().buf(), data->getContent().size());
  string prefix = QString::fromStdString (originPrefix).trimmed ().toUtf8().constData();
  string randomSuffix = getRandomString();
  m_localPrefix = Name(prefix);
  
}

void
ContactPanel::onLocalPrefixTimeout(const shared_ptr<const Interest>& interest,
                                   int retry)
{ 
  if(retry > 0)
    {
      setLocalPrefix(retry - 1);
      return;
    }
  else{
    m_localPrefix = Name("/private/local");
  }
}

void
ContactPanel::setInvitationListener()
{
  m_inviteListenPrefix = Name("/ndn/broadcast/chronos/invitation");
  m_inviteListenPrefix.append(m_defaultIdentity);
  _LOG_DEBUG("Listening for invitation on prefix: " << m_inviteListenPrefix.toUri());
  m_invitationListenerId = m_face->registerPrefix(m_inviteListenPrefix, 
                                                  boost::bind(&ContactPanel::onInvitation, this, _1, _2, _3, _4),
                                                  boost::bind(&ContactPanel::onInvitationRegisterFailed, this, _1));
}

void
ContactPanel::sendInterest(const Interest& interest,
                           const OnVerified& onVerified,
                           const OnVerifyFailed& onVerifyFailed,
                           const TimeoutNotify& timeoutNotify,
                           int retry /* = 1 */,
                           int stepCount /* = 0 */)
{
  m_face->expressInterest(interest, 
                          boost::bind(&ContactPanel::onTargetData, 
                                      this,
                                      _1,
                                      _2,
                                      stepCount,
                                      onVerified, 
                                      onVerifyFailed,
                                      timeoutNotify),
                          boost::bind(&ContactPanel::onTargetTimeout,
                                      this,
                                      _1,
                                      retry,
                                      stepCount,
                                      onVerified,
                                      onVerifyFailed,
                                      timeoutNotify));
}

void
ContactPanel::onTargetData(const shared_ptr<const ndn::Interest>& interest, 
                           const shared_ptr<Data>& data,
                           int stepCount,
                           const OnVerified& onVerified,
                           const OnVerifyFailed& onVerifyFailed,
                           const TimeoutNotify& timeoutNotify)
{
  shared_ptr<ValidationRequest> nextStep = m_policyManager->checkVerificationPolicy(data, stepCount, onVerified, onVerifyFailed);

  if (nextStep)
    m_face->expressInterest
      (*nextStep->interest_, 
       bind(&ContactPanel::onCertData, this, _1, _2, nextStep), 
       bind(&ContactPanel::onCertTimeout, this, _1, onVerifyFailed, data, nextStep));

}

void
ContactPanel::onTargetTimeout(const shared_ptr<const ndn::Interest>& interest, 
                              int retry,
                              int stepCount,
                              const OnVerified& onVerified,
                              const OnVerifyFailed& onVerifyFailed,
                              const TimeoutNotify& timeoutNotify)
{
  if(retry > 0)
    sendInterest(*interest, onVerified, onVerifyFailed, timeoutNotify, retry-1, stepCount);
  else
    {
      _LOG_DEBUG("Interest: " << interest->getName().toUri() << " eventually times out!");
      timeoutNotify();
    }
}

void
ContactPanel::onCertData(const shared_ptr<const ndn::Interest>& interest, 
                         const shared_ptr<Data>& cert,
                         shared_ptr<ValidationRequest> previousStep)
{
  shared_ptr<ValidationRequest> nextStep = m_policyManager->checkVerificationPolicy(cert, 
                                                                                    previousStep->stepCount_, 
                                                                                    previousStep->onVerified_, 
                                                                                    previousStep->onVerifyFailed_);

  if (nextStep)
    m_face->expressInterest
      (*nextStep->interest_, 
       bind(&ContactPanel::onCertData, this, _1, _2, nextStep), 
       bind(&ContactPanel::onCertTimeout, this, _1, previousStep->onVerifyFailed_, cert, nextStep));
}

void
ContactPanel::onCertTimeout(const shared_ptr<const ndn::Interest>& interest,
                            const OnVerifyFailed& onVerifyFailed,
                            const shared_ptr<Data>& data,
                            shared_ptr<ValidationRequest> nextStep)
{
  if(nextStep->retry_ > 0)
    m_face->expressInterest(*interest, 
                            bind(&ContactPanel::onCertData,
                                 this,
                                 _1,
                                 _2,
                                 nextStep),
                            bind(&ContactPanel::onCertTimeout,
                                 this,
                                 _1,
                                 onVerifyFailed,
                                 data,
                                 nextStep));
 else
   onVerifyFailed(data);
}

void
ContactPanel::onInvitationRegisterFailed(const shared_ptr<const Name>& prefix)
{
  showError(QString::fromStdString("Cannot register invitation listening prefix"));
}

void
ContactPanel::onInvitation(const shared_ptr<const Name>& prefix, 
                           const shared_ptr<const Interest>& interest, 
                           Transport& transport, 
                           uint64_t registeredPrefixId)
{
  _LOG_DEBUG("Receive invitation!" << interest->getName().toUri());

  
  shared_ptr<ChronosInvitation> invitation;
  try{
    invitation = make_shared<ChronosInvitation>(interest->getName());
  }catch(std::exception& e){
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

  shared_ptr<PublicKey> keyPtr = m_policyManager->getTrustedKey(invitation->getInviterCertificateName());
  if(CHRONOCHAT_NULL_PUBLICKEY_PTR != keyPtr && Sha256WithRsaHandler::verifySignature(invitation->getSignedBlob(), invitation->getSignatureBits(), *keyPtr))
    {
      shared_ptr<IdentityCertificate> certificate = make_shared<IdentityCertificate>();
      // hack: incomplete certificate, we don't send it to the wire nor store it anywhere, we only use it to carry information
      certificate->setName(invitation->getInviterCertificateName());
      bool findCert = false;
      vector<shared_ptr<ContactItem> >::const_iterator it = m_contactList.begin();
      for(; it != m_contactList.end(); it++)
        {
          if((*it)->getNameSpace() == invitation->getInviterNameSpace())
            {
              certificate->setNotBefore((*it)->getSelfEndorseCertificate().getNotBefore());
              certificate->setNotAfter((*it)->getSelfEndorseCertificate().getNotAfter());
              findCert = true;
              break;
            }
        }
      if(findCert == false)
        {
          _LOG_ERROR("No SelfEndorseCertificate found!");
          return;
        }
      certificate->setPublicKeyInfo(*keyPtr);
      popChatInvitation(invitation, invitation->getInviterNameSpace(), certificate);
      return;
    }

  _LOG_DEBUG("Cannot find the inviter's key in trust anchors");

  Interest newInterest(invitation->getInviterCertificateName());
  OnVerified onVerified = boost::bind(&ContactPanel::onInvitationCertVerified, this, _1, invitation);
  OnVerifyFailed onVerifyFailed = boost::bind(&ContactPanel::onInvitationCertVerifyFailed, this, _1);
  TimeoutNotify timeoutNotify = boost::bind(&ContactPanel::onInvitationCertTimeoutNotify, this);

  sendInterest(newInterest, onVerified, onVerifyFailed, timeoutNotify);
}

void
ContactPanel::onInvitationCertVerified(const shared_ptr<Data>& data, 
                                       shared_ptr<ChronosInvitation> invitation)
{
  shared_ptr<IdentityCertificate> certificate = make_shared<IdentityCertificate>(*data);
  
  if(Sha256WithRsaHandler::verifySignature(invitation->getSignedBlob(), invitation->getSignatureBits(), certificate->getPublicKeyInfo()))
    {
      Name keyName = certificate->getPublicKeyName();
      Name inviterNameSpace = keyName.getPrefix(-1);
      popChatInvitation(invitation, inviterNameSpace, certificate);
    }
}

void
ContactPanel::onInvitationCertVerifyFailed(const shared_ptr<Data>& data)
{ _LOG_DEBUG("Cannot verify invitation certificate!"); }

void
ContactPanel::onInvitationCertTimeoutNotify()
{ _LOG_DEBUG("interest for invitation certificate times out eventually!"); }

void
ContactPanel::popChatInvitation(shared_ptr<ChronosInvitation> invitation,
                                const Name& inviterNameSpace,
                                shared_ptr<IdentityCertificate> certificate)
{
  string alias;
  vector<shared_ptr<ContactItem> >::iterator it = m_contactList.begin();
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
  m_collectStatus = make_shared<vector<bool> >();
  m_collectStatus->assign(m_contactList.size(), false);

  vector<shared_ptr<ContactItem> >::iterator it = m_contactList.begin();
  int count = 0;
  for(; it != m_contactList.end(); it++, count++)
    {
      Name interestName = (*it)->getNameSpace();
      interestName.append("DNS").append(m_defaultIdentity).append("ENDORSEE");
      Interest interest(interestName);
      interest.setChildSelector(ndn_Interest_CHILD_SELECTOR_RIGHT);
      interest.setInterestLifetimeMilliseconds(1000);

      OnVerified onVerified = boost::bind(&ContactPanel::onDnsEndorseeVerified, this, _1, count);
      OnVerifyFailed onVerifyFailed = boost::bind(&ContactPanel::onDnsEndorseeVerifyFailed, this, _1, count);
      TimeoutNotify timeoutNotify = boost::bind(&ContactPanel::onDnsEndorseeTimeoutNotify, this, count);
  
      sendInterest(interest, onVerified, onVerifyFailed, timeoutNotify, 0);
    }
}

void
ContactPanel::onDnsEndorseeVerified(const shared_ptr<Data>& data, int count)
{
  Data endorseData;
  endorseData.wireDecode(data->getContent().buf(), data->getContent().size());
  EndorseCertificate endorseCertificate(endorseData);

  m_contactManager->getContactStorage()->updateCollectEndorse(endorseCertificate);

  updateCollectStatus(count);
}

void
ContactPanel::onDnsEndorseeTimeoutNotify(int count)
{ updateCollectStatus(count); }

void
ContactPanel::onDnsEndorseeVerifyFailed(const shared_ptr<Data>& data, int count)
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
  Name defaultCertName = m_identityManager->getDefaultCertificateNameForIdentity(defaultIdentity);
  if(defaultCertName.size() == 0)
    {
      showWarning(QString::fromStdString("Corresponding certificate is missing!\nHave you installed the certificate?"));
      return;
    }
  m_defaultIdentity = defaultIdentity;
  m_profileEditor->setCurrentIdentity(m_defaultIdentity);
  m_nickName = nickName.toStdString();
  m_face->removeRegisteredPrefix(m_invitationListenerId);
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
ContactPanel::openInvitationDialog()
{ m_invitationDialog->show(); }

void
ContactPanel::addContactIntoPanelPolicy(const Name& contactNameSpace)
{
  shared_ptr<ContactItem> contact = m_contactManager->getContact(contactNameSpace);
  if(contact != CHRONOCHAT_NULL_CONTACTITEM_PTR)
    m_policyManager->addTrustAnchor(contact->getSelfEndorseCertificate());
}

void
ContactPanel::removeContactFromPanelPolicy(const Name& keyName)
{ m_policyManager->removeTrustAnchor(keyName); }

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
  Name chatroomName(chatroom.toStdString());

  Name inviteeNamespace(invitee.toStdString());
  shared_ptr<ContactItem> inviteeItem = m_contactManager->getContact(inviteeNamespace);

  ChatDialog* chatDialog = new ChatDialog(m_contactManager, m_identityManager, chatroomName, m_localPrefix, m_defaultIdentity, m_nickName);
  m_chatDialogs.insert(pair <Name, ChatDialog*> (chatroomName, chatDialog));

  connect(chatDialog, SIGNAL(closeChatDialog(const ndn::Name&)),
          this, SLOT(removeChatDialog(const ndn::Name&)));
  connect(chatDialog, SIGNAL(noNdnConnection(const QString&)),
          this, SLOT(showError(const QString&)));
  connect(chatDialog, SIGNAL(inivationRejection(const QString&)),
          this, SLOT(showWarning(const QString&)));

  // send invitation
  chatDialog->sendInvitation(inviteeItem, isIntroducer); 
  
  chatDialog->show();
}

// For Invitee
void
ContactPanel::startChatroom2(const ChronosInvitation& invitation, 
                             const IdentityCertificate& identityCertificate)
{
  shared_ptr<ContactItem> inviterItem = m_contactManager->getContact(invitation.getInviterNameSpace());

  Name chatroomName("/ndn/broadcast/chronos");
  chatroomName.append(invitation.getChatroom());

  ChatDialog* chatDialog = new ChatDialog(m_contactManager, m_identityManager, chatroomName, m_localPrefix, m_defaultIdentity, m_nickName, true);

  connect(chatDialog, SIGNAL(closeChatDialog(const ndn::Name&)),
          this, SLOT(removeChatDialog(const ndn::Name&)));
  connect(chatDialog, SIGNAL(noNdnConnection(const QString&)),
          this, SLOT(showError(const QString&)));
  connect(chatDialog, SIGNAL(inivationRejection(const QString&)),
          this, SLOT(showWarning(const QString&)));

  chatDialog->addChatDataRule(invitation.getInviterPrefix(), identityCertificate, true);
  chatDialog->publishIntroCert(identityCertificate, true);

  chatDialog->addTrustAnchor(inviterItem->getSelfEndorseCertificate());
  
  m_chatDialogs.insert(pair <Name, ChatDialog*> (chatroomName, chatDialog));

  chatDialog->show();
}

void
ContactPanel::acceptInvitation(const ChronosInvitation& invitation, 
                               const IdentityCertificate& identityCertificate)
{
  Name dataName = invitation.getInterestName();
  time_t nowSeconds = time(NULL);
  struct tm current = *gmtime(&nowSeconds);
  MillisecondsSince1970 version = timegm(&current) * 1000.0;
  dataName.appendVersion(version);
  Data data(dataName);
  string content = m_localPrefix.toUri();
  data.setContent((const uint8_t *)&content[0], content.size());
  data.getMetaInfo().setTimestampMilliseconds(time(NULL) * 1000.0);

  Name certificateName;
  Name inferredIdentity = m_policyManager->inferSigningIdentity(data.getName());

  if(inferredIdentity.getComponentCount() == 0)
    certificateName = m_identityManager->getDefaultCertificateName();
  else
    certificateName = m_identityManager->getDefaultCertificateNameForIdentity(inferredIdentity);   
  m_identityManager->signByCertificate(data, certificateName);

  m_transport->send(*data.wireEncode());

  startChatroom2(invitation, identityCertificate);
}

void
ContactPanel::rejectInvitation(const ChronosInvitation& invitation)
{
  Data data(invitation.getInterestName());
  string content("nack");
  data.setContent((const uint8_t *)&content[0], content.size());
  data.getMetaInfo().setTimestampMilliseconds(time(NULL) * 1000.0);

  Name certificateName;
  Name inferredIdentity = m_policyManager->inferSigningIdentity(data.getName());
  if(inferredIdentity.getComponentCount() == 0)
    certificateName = m_identityManager->getDefaultCertificateName();
  else
    certificateName = m_identityManager->getDefaultCertificateNameForIdentity(inferredIdentity);   
  m_identityManager->signByCertificate(data, certificateName);

  m_transport->send(*data.wireEncode());
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
