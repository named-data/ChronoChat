/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */


#include "invitation-dialog.h"
#include "ui_invitation-dialog.h"

using namespace ndn;

InvitationDialog::InvitationDialog(QWidget *parent) 
  : QDialog(parent)
  , ui(new Ui::InvitationDialog)
{
    ui->setupUi(this);
    
    connect(ui->okButton, SIGNAL(clicked()),
            this, SLOT(onOkClicked()));
    connect(ui->cancelButton, SIGNAL(clicked()),
            this, SLOT(onCancelClicked()));
}

InvitationDialog::~InvitationDialog()
{
    delete ui;
}

void
InvitationDialog::setInvitation(const std::string& alias,
                                const std::string& chatroom,
                                const Name& interestName)
{
  std::string msg = alias;
  m_invitationInterest = interestName;
  msg.append(" invites you to: ").append(chatroom);
  ui->msgLabel->setText(QString::fromStdString(msg));
}

void
InvitationDialog::onOkClicked()
{ 
  emit invitationResponded(m_invitationInterest, true); 
  this->close();

  ui->msgLabel->clear();
  m_invitationInterest.clear();
}
  
void
InvitationDialog::onCancelClicked()
{ 
  emit invitationResponded(m_invitationInterest, false);
  this->close();

  ui->msgLabel->clear();
  m_invitationInterest.clear();
}

#if WAF
#include "invitation-dialog.moc"
#include "invitation-dialog.cpp.moc"
#endif
