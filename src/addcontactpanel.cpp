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

using namespace ndn;
using namespace std;

AddContactPanel::AddContactPanel(Ptr<ContactManager> contactManager,
                                 QWidget *parent) 
  : QDialog(parent)
  , ui(new Ui::AddContactPanel)
  , m_contactManager(contactManager)
{
  ui->setupUi(this);

  connect(ui->cancelButton, SIGNAL(clicked()),
          this, SLOT(onCancelClicked()));
  connect(ui->searchButton, SIGNAL(clicked()),
          this, SLOT(onSearchClicked()));
  connect(&*m_contactManager, SIGNAL(contactFetched(Ptr<EndorseCertificate>)),
          this, SLOT(selfEndorseCertificateFetched(Ptr<EndorseCertificate>)));
}

AddContactPanel::~AddContactPanel()
{
    delete ui;
}

void
AddContactPanel::onCancelClicked()
{ this->close(); }

void
AddContactPanel::onSearchClicked()
{
  QString inputIdentity = ui->contactInput->text();
  m_searchIdentity = Name(inputIdentity.toUtf8().constData());
}

void
AddContactPanel::onAddClicked()
{
}

void 
AddContactPanel::selfEndorseCertificateFetched(Ptr<EndorseCertificate> endorseCertificate)
{
}

#if WAF
#include "addcontactpanel.moc"
#include "addcontactpanel.cpp.moc"
#endif
