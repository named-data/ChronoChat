/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "setaliasdialog.h"
#include "ui_setaliasdialog.h"


using namespace ndn;
using namespace std;

SetAliasDialog::SetAliasDialog(Ptr<ContactManager> contactManager,
			       QWidget *parent) 
  : QDialog(parent)
  , ui(new Ui::SetAliasDialog)
  , m_contactManager(contactManager)
{
    ui->setupUi(this);

    connect(ui->okButton, SIGNAL(clicked()),
	    this, SLOT(onOkClicked()));
    connect(ui->cancelButton, SIGNAL(clicked()),
	    this, SLOT(onCancelClicked()));
}

SetAliasDialog::~SetAliasDialog()
{
    delete ui;
}

void
SetAliasDialog::onOkClicked()
{
  QString text = ui->aliasInput->text();
  string alias = text.toUtf8().constData();
  
  m_contactManager->getContactStorage()->updateAlias(Name(m_target), alias);

  emit aliasChanged();

  this->close();
}

void
SetAliasDialog::onCancelClicked()
{ this->close(); }

void 
SetAliasDialog::setTargetIdentity(const string& name)
{ 
  m_target = name; 
  string msg("Set alias for ");
  msg.append(name).append(":");
  ui->introLabel->setText(QString::fromUtf8(msg.c_str()));
  ui->aliasInput->clear();
}


#if WAF
#include "setaliasdialog.moc"
#include "setaliasdialog.cpp.moc"
#endif
