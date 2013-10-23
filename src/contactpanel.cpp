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
#include <ndn.cxx/security/identity/osx-privatekey-storage.h>
#include <ndn.cxx/security/identity/identity-manager.h>
#include <ndn.cxx/security/identity/basic-identity-storage.h>
#include <ndn.cxx/security/cache/ttl-certificate-cache.h>
#include <ndn.cxx/security/encryption/basic-encryption-manager.h>
#include <ndn.cxx/common.h>
#include <boost/filesystem.hpp>
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include "invitation-policy-manager.h"
#include "logging.h"
#include "exception.h"
#endif

namespace fs = boost::filesystem;
using namespace ndn;

using namespace std;

INIT_LOGGER("ContactPanel");

Q_DECLARE_METATYPE(ndn::security::IdentityCertificate)

ContactPanel::ContactPanel(Ptr<ContactManager> contactManager, QWidget *parent) 
    : QDialog(parent)
    , ui(new Ui::ContactPanel)
    , m_contactManager(contactManager)
    , m_contactListModel(new QStringListModel)
    , m_profileEditor(new ProfileEditor(m_contactManager))
    , m_addContactPanel(new AddContactPanel(contactManager))
    , m_setAliasDialog(new SetAliasDialog(contactManager))
    , m_startChatDialog(new StartChatDialog)
    , m_invitationDialog(new InvitationDialog)
    , m_settingDialog(new SettingDialog)
    , m_menuInvite(new QAction("&Chat", this))
    , m_menuAlias(new QAction("&Set Alias", this))
{
  qRegisterMetaType<ndn::security::IdentityCertificate>("IdentityCertificate");
  
   ui->setupUi(this);
   refreshContactList();

   openDB();    

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

   connect(m_invitationDialog, SIGNAL(invitationAccepted(const ndn::Name&, const ndn::security::IdentityCertificate&, QString, QString)),
           this, SLOT(acceptInvitation(const ndn::Name&, const ndn::security::IdentityCertificate&, QString, QString)));
   connect(m_invitationDialog, SIGNAL(invitationRejected(const ndn::Name&)),
           this, SLOT(rejectInvitation(const ndn::Name&)));

   connect(m_settingDialog, SIGNAL(identitySet(const QString&)),
           this, SLOT(updateDefaultIdentity(const QString&)));



}

ContactPanel::~ContactPanel()
{
    delete ui;
    delete m_contactListModel;
    delete m_profileEditor;
    delete m_addContactPanel;

    delete m_menuInvite;
}

void
ContactPanel::openDB()
{
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
  QString path = (QDir::home().path());
  path.append(QDir::separator()).append(".chronos").append(QDir::separator()).append("chronos.db");
  db.setDatabaseName(path);
  bool ok = db.open();
}

void 
ContactPanel::setKeychain()
{
  Ptr<security::OSXPrivatekeyStorage> privateStorage = Ptr<security::OSXPrivatekeyStorage>::Create();
  Ptr<security::IdentityManager> identityManager = Ptr<security::IdentityManager>(new security::IdentityManager(Ptr<security::BasicIdentityStorage>::Create(), privateStorage));
  Ptr<security::CertificateCache> certificateCache = Ptr<security::CertificateCache>(new security::TTLCertificateCache());
  Ptr<InvitationPolicyManager> policyManager = Ptr<InvitationPolicyManager>(new InvitationPolicyManager(10, certificateCache));
  Ptr<security::EncryptionManager> encryptionManager = Ptr<security::EncryptionManager>(new security::BasicEncryptionManager(privateStorage, "/tmp/encryption.db"));

  vector<Ptr<ContactItem> >::const_iterator it = m_contactList.begin();
  for(; it != m_contactList.end(); it++)
      policyManager->addTrustAnchor((*it)->getSelfEndorseCertificate());

  m_keychain = Ptr<security::Keychain>(new security::Keychain(identityManager, policyManager, encryptionManager));
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
  _LOG_DEBUG("prefix: " << prefix);
  _LOG_DEBUG("randomSuffix: " << randomSuffix);
  m_localPrefix = Name(prefix);
}

void
ContactPanel::onLocalPrefixTimeout(Ptr<Closure> closure, Ptr<Interest> interest)
{ throw LnException("No local prefix is found!"); }

void
ContactPanel::onUnverified(Ptr<Data> data)
{}

void
ContactPanel::onTimeout(Ptr<Closure> closure, Ptr<Interest> interest)
{}

void
ContactPanel::onInvitationCertVerified(Ptr<Data> data, 
                                       const Name& interestName,
                                       int inviterIndex)
{
  Ptr<security::IdentityCertificate> certificate = Ptr<security::IdentityCertificate>(new security::IdentityCertificate(*data));

  const int end = interestName.size();

  string signature = interestName.get(end-1).toBlob();
  Blob signatureBlob(signature.c_str(), signature.size());
  string signedName = interestName.getSubName(0, end - 1).toUri();
  Blob signedBlob(signedName.c_str(), signedName.size());

  if(security::PolicyManager::verifySignature(signedBlob, signatureBlob, certificate->getPublicKeyInfo()))
    {
      Name keyName = certificate->getPublicKeyName();
      Name inviterNameSpace = keyName.getSubName(0, keyName.size() - 1);
      popChatInvitation(interestName, inviterIndex, inviterNameSpace, certificate);
    }
}

void
ContactPanel::popChatInvitation(const Name& interestName,
                                int inviterIndex,
                                const Name& inviterNameSpace,
                                Ptr<security::IdentityCertificate> certificate)
{
  string chatroomTag("chatroom");
  int i = 0;
  for(; i < inviterIndex; i++)
    if(interestName.get(i).toUri() == chatroomTag)
      break;
  if(i+1 >= inviterIndex)
    return;

  string chatroom = interestName.get(i+1).toUri();
  string inviter = inviterNameSpace.toUri();
  
  m_invitationDialog->setMsg(inviter, chatroom);
  m_invitationDialog->setIdentityCertificate(certificate);
  m_invitationDialog->setInterestName(interestName);
  m_invitationDialog->show();
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
  m_handler->setInterestFilter (prefix, 
                                boost::bind(&ContactPanel::onInvitation, 
                                            this,
                                            _1));

}

void
ContactPanel::onInvitation(Ptr<Interest> interest)
{
  const Name& interestName = interest->getName();
  const int end = interestName.size();
  
  string inviter("inviter");
  int j = end-2;
  for(; j >= 0; j--)
    if(interestName.get(j).toUri() == inviter)
      break;

  //No certificate name found
  if(j < 0)
    return;
  
  Name certName = interestName.getSubName(j+1, end-2-j);
  string keyString("KEY");
  string idString("ID-CERT");
  int m = certName.size() - 1;
  int keyIndex = -1;
  int idIndex = -1;
  for(; m >= 0; m--)
    if(certName.get(m).toUri() == idString)
      {
        idIndex = m;
        break;
      }

  for(; m >=0; m--)
    if(certName.get(m).toUri() == keyString)
      {
        keyIndex = m;
        break;
      }

  //Not a qualified certificate name 
  if(keyIndex < 0 && idIndex < 0)
    return;

  Ptr<Interest> newInterest = Ptr<Interest>(new Interest(certName));
  Ptr<Closure> closure = Ptr<Closure>(new Closure(boost::bind(&ContactPanel::onInvitationCertVerified, 
                                                              this,
                                                              _1,
                                                              interestName,
                                                              j),
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
  
  QString name = QString::fromUtf8(m_contactList[i]->getName().c_str());
  QString institution = QString::fromUtf8(m_contactList[i]->getInstitution().c_str());
  QString nameSpace = QString::fromUtf8(m_contactList[i]->getNameSpace().toUri().c_str());
  ui->NameData->setText(name);
  ui->NameSpaceData->setText(nameSpace);
  ui->InstitutionData->setText(institution);

  m_currentSelectedContactAlias = alias;
  m_currentSelectedContactNamespace = m_contactList[i]->getNameSpace().toUri();
  // _LOG_DEBUG("current Alias: " << m_currentSelectedContact);
}

void
ContactPanel::updateDefaultIdentity(const QString& identity)
{ m_defaultIdentity = Name(identity.toUtf8().constData()); }

void
ContactPanel::openProfileEditor()
{ m_profileEditor->show(); }

void
ContactPanel::openAddContactPanel()
{ m_addContactPanel->show(); }

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
  m_setAliasDialog->setTargetIdentity(m_currentSelectedContactNamespace);
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
  TimeInterval ti = time::NowUnixTimestamp();
  ostringstream oss;
  oss << ti.total_seconds();

  Name chatroom("/ndn/broadcast/chronos");
  chatroom.append(string("chatroom-") + oss.str());

  m_startChatDialog->setInvitee(m_currentSelectedContactNamespace, chatroom.toUri());
  m_startChatDialog->show();
}

void
ContactPanel::startChatroom(const QString& chatroom, const QString& invitee, bool isIntroducer)
{
  _LOG_DEBUG("room: " << chatroom.toUtf8().constData());
  _LOG_DEBUG("invitee: " << invitee.toUtf8().constData());
  _LOG_DEBUG("introducer: " << std::boolalpha << isIntroducer);
}

void
ContactPanel::startChatroom2(const QString& chatroom, const QString& inviter)
{
  _LOG_DEBUG("room: " << chatroom.toUtf8().constData());
  _LOG_DEBUG("inviter: " << inviter.toUtf8().constData());
}

void
ContactPanel::acceptInvitation(const Name& interestName, 
                               const security::IdentityCertificate& identityCertificate, 
                               QString inviter, 
                               QString chatroom)
{
  string prefix = m_localPrefix.toUri();
  m_handler->publishDataByIdentity (interestName, prefix);
  //TODO:: open chat dialog
  startChatroom2(chatroom, inviter);
}

void
ContactPanel::rejectInvitation(const ndn::Name& interestName)
{
  string empty;
  m_handler->publishDataByIdentity (interestName, empty);
}


#if WAF
#include "contactpanel.moc"
#include "contactpanel.cpp.moc"
#endif
