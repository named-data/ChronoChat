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

AddContactPanel::AddContactPanel(QWidget *parent) 
  : QDialog(parent)
  , ui(new Ui::AddContactPanel)
  , m_selfEndorseCertificate(NULL)
{
  ui->setupUi(this);

  connect(ui->cancelButton, SIGNAL(clicked()),
          this, SLOT(onCancelClicked()));
  connect(ui->searchButton, SIGNAL(clicked()),
          this, SLOT(onSearchClicked()));
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
}

void
AddContactPanel::onAddClicked()
{
}

#if WAF
#include "addcontactpanel.moc"
#include "addcontactpanel.cpp.moc"
#endif
