/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "addcontactpanel.h"
#include "ui_addcontactpanel.h"

#ifndef Q_MOC_RUN
#include <cryptopp/base64.h>
// #include <ndn.cxx/helpers/der/der.h>
#include <ndn.cxx/helpers/der/visitor/simple-visitor.h>
#include "logging.h"
#endif

using namespace ndn;
using namespace std;

INIT_LOGGER("AddContactPanel");

AddContactPanel::AddContactPanel(Ptr<ContactManager> contactManager,
                                 QWidget *parent) 
  : QDialog(parent)
  , ui(new Ui::AddContactPanel)
  , m_contactManager(contactManager)
  , m_warningDialog(new WarningDialog())
{
  ui->setupUi(this);
  
  qRegisterMetaType<ndn::Name>("NdnName");
  qRegisterMetaType<EndorseCertificate>("EndorseCertificate");
  qRegisterMetaType<ndn::Data>("NdnData");

  connect(ui->cancelButton, SIGNAL(clicked()),
          this, SLOT(onCancelClicked()));
  connect(ui->searchButton, SIGNAL(clicked()),
          this, SLOT(onSearchClicked()));
  connect(ui->addButton, SIGNAL(clicked()),
          this, SLOT(onAddClicked()));
  connect(&*m_contactManager, SIGNAL(contactFetched(const EndorseCertificate&)),
          this, SLOT(selfEndorseCertificateFetched(const EndorseCertificate&)));
  connect(&*m_contactManager, SIGNAL(contactFetchFailed(const ndn::Name&)),
          this, SLOT(selfEndorseCertificateFetchFailed(const ndn::Name&)));
  connect(&*m_contactManager, SIGNAL(collectEndorseFetched(const ndn::Data&)),
          this, SLOT(onCollectEndorseFetched(const ndn::Data&)));
  connect(&*m_contactManager, SIGNAL(collectEndorseFetchFailed(const ndn::Name&)),
          this, SLOT(onCollectEndorseFetchFailed(const ndn::Name&)));
  connect(&*m_contactManager, SIGNAL(contactKeyFetched(const EndorseCertificate&)),
          this, SLOT(onContactKeyFetched(const EndorseCertificate&)));
  connect(&*m_contactManager, SIGNAL(contactKeyFetchFailed(const ndn::Name&)),
          this, SLOT(onContactKeyFetchFailed(const ndn::Name&)));


}

AddContactPanel::~AddContactPanel()
{
    delete ui;
    delete m_warningDialog;
}

void
AddContactPanel::onCancelClicked()
{ this->close(); }

void
AddContactPanel::onSearchClicked()
{
  m_currentEndorseCertificateReady = false;
  m_currentCollectEndorseReady = false;
  ui->infoView->clear();
  for(int i = ui->infoView->rowCount() - 1; i >= 0 ; i--)
    ui->infoView->removeRow(i);
  QString inputIdentity = ui->contactInput->text();
  m_searchIdentity = Name(inputIdentity.toUtf8().constData());

  if(Qt::Checked == ui->fetchBox->checkState())
    {
      m_contactManager->fetchSelfEndorseCertificate(m_searchIdentity);
      m_contactManager->fetchCollectEndorse(m_searchIdentity);
    }
  else
    {
      if(isCorrectName(m_searchIdentity))
        m_contactManager->fetchKey(m_searchIdentity);
      else
        {
          m_warningDialog->setMsg("Wrong key certificate name!");
          m_warningDialog->show();
        }
    }
}

bool
AddContactPanel::isCorrectName(const Name& name)
{
  string key("KEY");
  string idCert("ID-CERT");

  if(name.get(-1).toUri() != idCert)
    return false;
  
  int keyIndex = -1;
  for(int i = 0; i < name.size(); i++)
    {
      if(name.get(i).toUri() == key)
        {
          keyIndex = i;
          break;
        }
    }
  
  if(keyIndex < 0)
    return false;
  else
    return true;
}

void
AddContactPanel::onAddClicked()
{
  ContactItem contactItem(*m_currentEndorseCertificate);
  m_contactManager->getContactStorage()->addContact(contactItem);
  emit newContactAdded();
  this->close();
}

void 
AddContactPanel::selfEndorseCertificateFetched(const EndorseCertificate& endorseCertificate)
{
  m_currentEndorseCertificate = Ptr<EndorseCertificate>(new EndorseCertificate(endorseCertificate));
  m_currentEndorseCertificateReady = true;

  if(m_currentCollectEndorseReady == true)
    displayContactInfo();
}

void
AddContactPanel::selfEndorseCertificateFetchFailed(const Name& identity)
{
  m_warningDialog->setMsg("Cannot fetch contact profile");
  m_warningDialog->show();
}

void
AddContactPanel::onContactKeyFetched(const EndorseCertificate& endorseCertificate)
{
  m_currentEndorseCertificate = Ptr<EndorseCertificate>(new EndorseCertificate(endorseCertificate));

  m_currentCollectEndorseReady = NULL;

  displayContactInfo();
}

void
AddContactPanel::onContactKeyFetchFailed(const Name& identity)
{
  m_warningDialog->setMsg("Cannot fetch contact ksk certificate");
  m_warningDialog->show();
}

void
AddContactPanel::onCollectEndorseFetched(const Data& data)
{
  m_currentCollectEndorse = Ptr<Data>(new Data(data));
  m_currentCollectEndorseReady = true;

  if(m_currentEndorseCertificateReady == true)
    displayContactInfo();
}

void
AddContactPanel::onCollectEndorseFetchFailed(const Name& identity)
{
  m_currentCollectEndorse = NULL;
  m_currentCollectEndorseReady = true;
  
  if(m_currentEndorseCertificateReady == true)
    displayContactInfo();
}

void
AddContactPanel::displayContactInfo()
{
  _LOG_TRACE("displayContactInfo");
  const Profile& profile = m_currentEndorseCertificate->getProfileData()->getProfile();
  const Blob& profileBlob = m_currentEndorseCertificate->getProfileData()->content();

  map<string, int> endorseCount;

  if(m_currentCollectEndorse != NULL)
    {
      _LOG_DEBUG("CollectEndorse fetched");
      boost::iostreams::stream
        <boost::iostreams::array_source> is (m_currentCollectEndorse->content().buf(), m_currentCollectEndorse->content().size());
  
      Ptr<der::DerSequence> root = DynamicCast<der::DerSequence>(der::DerNode::parse(reinterpret_cast<InputIterator &>(is)));
      const der::DerNodePtrList & children = root->getChildren();
      der::SimpleVisitor simpleVisitor;
      
      for(int i = 0; i < children.size(); i++)
        {
          Ptr<Blob> dataBlob = boost::any_cast<Ptr<Blob> >(children[i]->accept(simpleVisitor));
          Ptr<Data> data = Data::decodeFromWire(dataBlob);
          Ptr<EndorseCertificate> endorseCert = Ptr<EndorseCertificate>(new EndorseCertificate(*data));
          _LOG_DEBUG("endorseCert name: " << endorseCert->getName().toUri());
          
          Name signerKeyName = endorseCert->getSigner();
          Name signerName = signerKeyName.getPrefix(signerKeyName.size()-1);
          
          Ptr<ContactItem> contact = m_contactManager->getContact(signerName);
          if(contact == NULL)
            continue;
          _LOG_DEBUG("get contact: " << signerName.toUri());

          if(!contact->isIntroducer() || !contact->canBeTrustedFor(m_currentEndorseCertificate->getProfileData()->getIdentityName()))
            continue;
          _LOG_DEBUG("contact can be trusted");
          
          if(!security::PolicyManager::verifySignature(*data, contact->getSelfEndorseCertificate().getPublicKeyInfo()))
            continue;

          const Blob& tmpProfileBlob = endorseCert->getProfileData()->content();
          if(profileBlob != tmpProfileBlob)
            continue;

           _LOG_DEBUG("Profile equal");

          const vector<string>& endorseList = endorseCert->getEndorseList();
          vector<string>::const_iterator it = endorseList.begin();
          for(; it != endorseList.end(); it++)
            {
              _LOG_DEBUG("Entry: " << *it);
              endorseCount[*it] += 1;
            }
        }
    }

  // map<string, int>::iterator tmp_it = endorseCount.begin();
  // for(; tmp_it != endorseCount.end(); tmp_it++)
  //   {
  //     _LOG_DEBUG("Entry: " << tmp_it->first << " " << tmp_it->second);
  //   }
  
  ui->infoView->setColumnCount(3);
  Profile::const_iterator it = profile.begin();
  int rowCount = 0;
  
  QTableWidgetItem *typeHeader = new QTableWidgetItem(QString::fromUtf8("Type"));
  ui->infoView->setHorizontalHeaderItem(0, typeHeader);
  QTableWidgetItem *valueHeader = new QTableWidgetItem(QString::fromUtf8("Value"));
  ui->infoView->setHorizontalHeaderItem(1, valueHeader);
  QTableWidgetItem *endorseHeader = new QTableWidgetItem(QString::fromUtf8("Endorse"));
  ui->infoView->setHorizontalHeaderItem(2, endorseHeader);
  
  for(; it != profile.end(); it++)
  {
    ui->infoView->insertRow(rowCount);  
    QTableWidgetItem *type = new QTableWidgetItem(QString::fromUtf8(it->first.c_str()));
    ui->infoView->setItem(rowCount, 0, type);
    
    string valueString(it->second.buf(), it->second.size());
    QTableWidgetItem *value = new QTableWidgetItem(QString::fromUtf8(valueString.c_str()));
    ui->infoView->setItem(rowCount, 1, value);
    
    map<string, int>::iterator map_it = endorseCount.find(it->first);
    QTableWidgetItem *endorse = NULL;
    if(map_it == endorseCount.end())
      endorse = new QTableWidgetItem(QString::number(0));
    else
      endorse = new QTableWidgetItem(QString::number(map_it->second));
    ui->infoView->setItem(rowCount, 2, endorse);

    rowCount++;
  }
}

#if WAF
#include "addcontactpanel.moc"
#include "addcontactpanel.cpp.moc"
#endif
