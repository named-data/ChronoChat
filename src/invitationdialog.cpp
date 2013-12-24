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
using namespace ndn::ptr_lib;

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
                                shared_ptr<ChronosInvitation> invitation, 
                                shared_ptr<IdentityCertificate> identityCertificate)
{
  m_inviterAlias = alias;
  string msg = alias;
  msg.append("\ninvites you to join the chat room: ");
  ui->msgLabel->setText(QString::fromStdString(msg));

  m_invitation = invitation;
  ui->chatroomLine->setText(QString::fromStdString(invitation->getChatroom().get(0).toEscapedString()));

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

  emit invitationRejected(*m_invitation); 

  m_invitation = make_shared<ChronosInvitation>();
  m_identityCertificate = make_shared<IdentityCertificate>();
  m_inviterAlias.clear();

  this->close();
}

#if WAF
#include "invitationdialog.moc"
#include "invitationdialog.cpp.moc"
#endif
