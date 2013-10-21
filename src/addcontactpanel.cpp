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
  ui->infoView->clear();
  for(int i = ui->infoView->rowCount() - 1; i >= 0 ; i--)
    ui->infoView->removeRow(i);
  QString inputIdentity = ui->contactInput->text();
  m_searchIdentity = Name(inputIdentity.toUtf8().constData());

  m_contactManager->fetchSelfEndorseCertificate(m_searchIdentity);
}

void
AddContactPanel::onAddClicked()
{
  ContactItem contactItem(*m_currentEndorseCertificate);
  m_contactManager->getContactStorage()->addNormalContact(contactItem);
  emit newContactAdded();
  this->close();
}

void 
AddContactPanel::selfEndorseCertificateFetched(const EndorseCertificate& endorseCertificate)
{
  m_currentEndorseCertificate = Ptr<EndorseCertificate>(new EndorseCertificate(endorseCertificate));
  const Profile& profile = endorseCertificate.getProfileData()->getProfile();
  ui->infoView->setColumnCount(3);
  Profile::const_iterator it = profile.begin();
  int rowCount = 0;
  for(; it != profile.end(); it++)
  {
    ui->infoView->insertRow(rowCount);  
    QTableWidgetItem *type = new QTableWidgetItem(QString::fromUtf8(it->first.c_str()));
    ui->infoView->setItem(rowCount, 0, type);
    string valueString(it->second.buf(), it->second.size());
    QTableWidgetItem *value = new QTableWidgetItem(QString::fromUtf8(valueString.c_str()));
    ui->infoView->setItem(rowCount, 1, value);
    rowCount++;
  }
}

void
AddContactPanel::selfEndorseCertificateFetchFailed(const Name& identity)
{
  m_warningDialog->setMsg("Cannot fetch contact profile");
  m_warningDialog->show();
}

#if WAF
#include "addcontactpanel.moc"
#include "addcontactpanel.cpp.moc"
#endif
