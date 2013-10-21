/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "startchatdialog.h"
#include "ui_startchatdialog.h"

using namespace std;

StartChatDialog::StartChatDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StartChatDialog)
{
    ui->setupUi(this);
    connect(ui->okButton, SIGNAL(clicked()),
	    this, SLOT(onOkClicked()));
    connect(ui->cancelButton, SIGNAL(clicked()),
	    this, SLOT(onCancelClicked()));
}

StartChatDialog::~StartChatDialog()
{
    delete ui;
}

void
StartChatDialog::setInvitee(const string& invitee, const string& chatroom)
{ 
  m_invitee = invitee;
  ui->chatroomInput->setText(QString::fromUtf8(chatroom.c_str()));
}

void 
StartChatDialog::onOkClicked()
{
  QString chatroom = ui->chatroomInput->text();
  QString invitee = QString::fromUtf8(m_invitee.c_str());
  bool isIntroducer = ui->introCheckBox->isChecked();
  emit chatroomConfirmed(chatroom, invitee, isIntroducer);
  this->close();
}

void
StartChatDialog::onCancelClicked()
{ this->close(); }


#if WAF
#include "startchatdialog.moc"
#include "startchatdialog.cpp.moc"
#endif
