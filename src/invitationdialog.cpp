/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */


#include "invitationdialog.h"
#include "ui_invitationdialog.h"

using namespace std;

InvitationDialog::InvitationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InvitationDialog)
{
    ui->setupUi(this);
}

InvitationDialog::~InvitationDialog()
{
    delete ui;
}

void
InvitationDialog::setMsg(const string& inviter, const string& chatroom)
{
  string msg = inviter;
  msg.append(" invites you to join the chat room: ");
  
  ui->msgLabel->setText(QString::fromUtf8(msg.c_str()));
  ui->chatroomLine->setText(QString::fromUtf8(msg.c_str()));
}

void
InvitationDialog::onOkClicked()
{ emit invitationAccepted(m_interestName); }
  
void
InvitationDialog::onCancelClicked()
{ 
  ui->msgLabel->clear();
  ui->chatroomLine->clear();
  m_interestName = Name();
  emit invitationRejected(m_interestName); 
}

#if WAF
#include "invitationdialog.moc"
#include "invitationdialog.cpp.moc"
#endif
