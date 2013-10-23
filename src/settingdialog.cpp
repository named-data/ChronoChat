/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "settingdialog.h"
#include "ui_settingdialog.h"

using namespace std;

SettingDialog::SettingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingDialog)
{
    ui->setupUi(this);

    connect(ui->okButton, SIGNAL(clicked()),
            this, SLOT(onOkClicked()));
}

SettingDialog::~SettingDialog()
{
    delete ui;
}

void
SettingDialog::setIdentity(const std::string& identity)
{ 
  m_identity = identity;
  ui->identityLine->setText(QString::fromUtf8(m_identity.c_str()));
}

void
SettingDialog::onOkClicked()
{
  QString text = ui->identityLine->text();
  string identity = text.toUtf8().constData();
  if(identity != m_identity)
    {
      m_identity = identity;
      emit identitySet(text);
    }
  this->close();
}


#if WAF
#include "settingdialog.moc"
#include "settingdialog.cpp.moc"
#endif
