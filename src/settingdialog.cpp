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
SettingDialog::setIdentity(const std::string& identity,
                           const std::string& nickName)
{ 
  m_identity = identity;
  ui->identityLine->setText(QString::fromStdString(m_identity));
  m_nickName = nickName;
  ui->nickLine->setText(QString::fromStdString(m_nickName));
}

void
SettingDialog::onOkClicked()
{
  QString qIdentity = ui->identityLine->text();
  string identity = qIdentity.toStdString();
  QString qNick = ui->nickLine->text();
  string nick = qNick.toStdString();

  if(identity != m_identity || nick != m_nickName)
    {
      m_identity = identity;
      m_nickName = nick;
      emit identitySet(qIdentity, qNick);
    }
  this->close();
}


#if WAF
#include "settingdialog.moc"
#include "settingdialog.cpp.moc"
#endif
