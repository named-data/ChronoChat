/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "invitation-dialog.hpp"
#include "ui_invitation-dialog.h"

namespace chronochat {

InvitationDialog::InvitationDialog(QWidget* parent)
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
InvitationDialog::onInvitationReceived(QString alias, QString chatroom, Name interestName)
{
  m_invitationInterest = interestName;

  QString msg = QString("%1 invites you to chatroom\n %2 ").arg(alias).arg(chatroom);
  ui->msgLabel->setText(msg);

  show();
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

} // namespace chronochat

#if WAF
#include "invitation-dialog.moc"
// #include "invitation-dialog.cpp.moc"
#endif
