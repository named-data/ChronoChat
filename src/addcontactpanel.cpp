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
#include <QMessageBox>

#ifndef Q_MOC_RUN
#include <ndn-cpp-dev/security/validator.hpp>
#include <boost/iostreams/stream.hpp>
#include "endorse-collection.pb.h"
#include "logging.h"
#endif

using namespace ndn;
using namespace chronos;
using namespace std;

INIT_LOGGER("AddContactPanel");

AddContactPanel::AddContactPanel(shared_ptr<ContactManager> contactManager,
                                 QWidget *parent) 
  : QDialog(parent)
  , ui(new Ui::AddContactPanel)
  , m_contactManager(contactManager)
  , m_warningDialog(new WarningDialog())
{
  ui->setupUi(this);
  
  qRegisterMetaType<ndn::Name>("NdnName");
  qRegisterMetaType<chronos::EndorseCertificate>("EndorseCertificate");
  qRegisterMetaType<ndn::Data>("NdnData");

  connect(ui->cancelButton, SIGNAL(clicked()),
          this, SLOT(onCancelClicked()));
  connect(ui->searchButton, SIGNAL(clicked()),
          this, SLOT(onSearchClicked()));
  connect(ui->addButton, SIGNAL(clicked()),
          this, SLOT(onAddClicked()));
  connect(&*m_contactManager, SIGNAL(contactFetched(const chronos::EndorseCertificate&)),
          this, SLOT(selfEndorseCertificateFetched(const chronos::EndorseCertificate&)));
  connect(&*m_contactManager, SIGNAL(contactFetchFailed(const ndn::Name&)),
          this, SLOT(selfEndorseCertificateFetchFailed(const ndn::Name&)));
  connect(&*m_contactManager, SIGNAL(collectEndorseFetched(const ndn::Data&)),
          this, SLOT(onCollectEndorseFetched(const ndn::Data&)));
  connect(&*m_contactManager, SIGNAL(collectEndorseFetchFailed(const ndn::Name&)),
          this, SLOT(onCollectEndorseFetchFailed(const ndn::Name&)));
  connect(&*m_contactManager, SIGNAL(contactKeyFetched(const chronos::EndorseCertificate&)),
          this, SLOT(onContactKeyFetched(const chronos::EndorseCertificate&)));
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
  m_searchIdentity = Name(inputIdentity.toStdString());

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
          QMessageBox::information(this, tr("Chronos"), QString::fromStdString("Wrong key certificate name!"));
        }
    }
}

bool
AddContactPanel::isCorrectName(const Name& name)
{
  if(name.get(-1).toEscapedString() != "ID-CERT")
    return false;
  
  int keyIndex = -1;
  for(int i = 0; i < name.size(); i++)
    {
      if(name.get(i).toEscapedString() == "KEY")
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
  try{
    m_contactManager->getContactStorage()->addContact(contactItem);
  }catch(ContactStorage::Error& e){
    QMessageBox::information(this, tr("Chronos"), QString::fromStdString(e.what()));
    _LOG_ERROR("Exception: " << e.what());
    return;
  }
  emit newContactAdded();
  this->close();
}

void 
AddContactPanel::selfEndorseCertificateFetched(const EndorseCertificate& endorseCertificate)
{

  m_currentEndorseCertificate = make_shared<EndorseCertificate>(endorseCertificate);

  m_currentEndorseCertificateReady = true;

  if(m_currentCollectEndorseReady == true)
    displayContactInfo();
}

void
AddContactPanel::selfEndorseCertificateFetchFailed(const Name& identity)
{
  QMessageBox::information(this, tr("Chronos"), QString::fromStdString("Cannot fetch contact profile"));
}

void
AddContactPanel::onContactKeyFetched(const EndorseCertificate& endorseCertificate)
{
  m_currentEndorseCertificate = make_shared<EndorseCertificate>(endorseCertificate);

  m_currentCollectEndorseReady = false;

  displayContactInfo();
}

void
AddContactPanel::onContactKeyFetchFailed(const Name& identity)
{
  QMessageBox::information(this, tr("Chronos"), QString::fromStdString("Cannot fetch contact ksk certificate"));
}

void
AddContactPanel::onCollectEndorseFetched(const Data& data)
{
  m_currentCollectEndorse = make_shared<Data>(data);

  m_currentCollectEndorseReady = true;

  if(m_currentEndorseCertificateReady == true)
    displayContactInfo();
}

void
AddContactPanel::onCollectEndorseFetchFailed(const Name& identity)
{
  m_currentCollectEndorse = shared_ptr<Data>();

  m_currentCollectEndorseReady = true;
  
  if(m_currentEndorseCertificateReady == true)
    displayContactInfo();
}

void
AddContactPanel::displayContactInfo()
{
  // _LOG_TRACE("displayContactInfo");
  const Profile& profile = m_currentEndorseCertificate->getProfile();

  map<string, int> endorseCount;

  if(!static_cast<bool>(m_currentCollectEndorse))
    {
      Chronos::EndorseCollection endorseCollection;
      
      boost::iostreams::stream
        <boost::iostreams::array_source> is (reinterpret_cast<const char*>(m_currentCollectEndorse->getContent().value()), 
                                             m_currentCollectEndorse->getContent().value_size());
      
      endorseCollection.ParseFromIstream(&is);

      for(int i = 0; i < endorseCollection.endorsement_size(); i ++)
        {
          try{
            Data data;
            data.wireDecode(Block(reinterpret_cast<const uint8_t*>(endorseCollection.endorsement(i).blob().c_str()),
                                  endorseCollection.endorsement(i).blob().size()));
            EndorseCertificate endorseCert(data);
            
            Name signerName = endorseCert.getSigner().getPrefix(-1);
          
            shared_ptr<ContactItem> contact = m_contactManager->getContact(signerName);
            if(!static_cast<bool>(contact))
              continue;

            if(!contact->isIntroducer() || !contact->canBeTrustedFor(m_currentEndorseCertificate->getProfile().getIdentityName()))
              continue;
          
            if(!Validator::verifySignature(data, data.getSignature(), contact->getSelfEndorseCertificate().getPublicKeyInfo()))
              continue;

            const Profile& tmpProfile = endorseCert.getProfile();
            if(!(profile == tmpProfile))
              continue;

          const vector<string>& endorseList = endorseCert.getEndorseList();
          vector<string>::const_iterator it = endorseList.begin();
          for(; it != endorseList.end(); it++)
            endorseCount[*it] += 1;
          }catch(std::runtime_error& e){
            continue;
          }
        }  
    }
  
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
    QTableWidgetItem *type = new QTableWidgetItem(QString::fromStdString(it->first));
    ui->infoView->setItem(rowCount, 0, type);
    
    QTableWidgetItem *value = new QTableWidgetItem(QString::fromStdString(it->second));
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
