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
using namespace ndn;

InvitationDialog::InvitationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InvitationDialog)
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
InvitationDialog::setInvitation(const string& alias,
                                Ptr<ChronosInvitation> invitation, 
                                Ptr<security::IdentityCertificate> identityCertificate)
{
  m_inviterAlias = alias;
  string msg = alias;
  msg.append(" invites you to join the chat room: ");
  ui->msgLabel->setText(QString::fromStdString(msg));

  m_invitation = invitation;
  ui->chatroomLine->setText(QString::fromStdString(invitation->getChatroom().get(0).toUri()));

  m_identityCertificate = identityCertificate;
}

void
InvitationDialog::onOkClicked()
{ 
  emit invitationAccepted(*m_invitation, *m_identityCertificate); 
  this->close();
}
  
void
InvitationDialog::onCancelClicked()
{ 
  ui->msgLabel->clear();
  ui->chatroomLine->clear();

  m_invitation = NULL;
  m_identityCertificate = NULL;
  m_inviterAlias.clear();

  emit invitationRejected(*m_invitation); 
  this->close();
}

#if WAF
#include "invitationdialog.moc"
#include "invitationdialog.cpp.moc"
#endif
