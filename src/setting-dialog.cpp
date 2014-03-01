/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "setting-dialog.h"
#include "ui_setting-dialog.h"

SettingDialog::SettingDialog(QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::SettingDialog)
{
  ui->setupUi(this);
  
  connect(ui->saveButton, SIGNAL(clicked()),
          this, SLOT(onSaveClicked()));
  connect(ui->cancelButton, SIGNAL(clicked()),
          this, SLOT(onCancelClicked()));
}

SettingDialog::~SettingDialog()
{
  delete ui;
}

void
SettingDialog::setNick(const QString& nick)
{
  m_nick = nick;
  ui->nickLine->setText(m_nick);
}

void
SettingDialog::onIdentityUpdated(const QString& identity)
{
  m_identity = identity;
  ui->identityLine->setText(m_identity);
}

void
SettingDialog::onSaveClicked()
{
  QString identity = ui->identityLine->text();
  QString nick = ui->nickLine->text();

  if(identity != m_identity)
    {
      m_identity = identity;
      emit identityUpdated(identity);
    }
  if(nick != m_nick)
    {
      m_nick = nick;
      emit nickUpdated(nick);
    }

  this->close();
}

void
SettingDialog::onCancelClicked()
{
  this->close();
}


#if WAF
#include "setting-dialog.moc"
#include "setting-dialog.cpp.moc"
#endif
