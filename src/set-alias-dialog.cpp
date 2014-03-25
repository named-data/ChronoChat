/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "set-alias-dialog.h"
#include "ui_set-alias-dialog.h"


SetAliasDialog::SetAliasDialog(QWidget *parent) 
  : QDialog(parent)
  , ui(new Ui::SetAliasDialog)
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
  emit aliasChanged(m_targetIdentity, ui->aliasInput->text());
  this->close();
}

void
SetAliasDialog::onCancelClicked()
{ 
  this->close(); 
}

void 
SetAliasDialog::setTargetIdentity(const QString& targetIdentity, const QString& alias)
{ 
  m_targetIdentity = targetIdentity; 
  QString msg = QString("Set alias for %1:").arg(targetIdentity);
  ui->introLabel->setText(msg);
  ui->aliasInput->setText(alias);
}


#if WAF
#include "set-alias-dialog.moc"
#include "set-alias-dialog.cpp.moc"
#endif
