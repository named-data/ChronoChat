/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 *         Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "invitation-request-dialog.hpp"
#include "ui_invitation-request-dialog.h"

namespace chronochat {

InvitationRequestDialog::InvitationRequestDialog(QWidget* parent)
  : QDialog(parent)
  , ui(new Ui::InvitationRequestDialog)
{
    ui->setupUi(this);

    connect(ui->okButton, SIGNAL(clicked()),
            this, SLOT(onOkClicked()));
    connect(ui->cancelButton, SIGNAL(clicked()),
            this, SLOT(onCancelClicked()));
}

InvitationRequestDialog::~InvitationRequestDialog()
{
    delete ui;
}

void
InvitationRequestDialog::onInvitationRequestReceived(QString alias, QString chatroom,
                                                     Name interestName)
{
  m_invitationInterest = interestName;

  QString msg = QString("%1\n request your invitation to chatroom\n %2 ").arg(alias).arg(chatroom);
  ui->msgLabel->setText(msg);

  show();
  raise();
}

void
InvitationRequestDialog::onOkClicked()
{
  emit invitationRequestResponded(m_invitationInterest, true);
  this->close();

  ui->msgLabel->clear();
  m_invitationInterest.clear();
}

void
InvitationRequestDialog::onCancelClicked()
{
  emit invitationRequestResponded(m_invitationInterest, false);
  this->close();

  ui->msgLabel->clear();
  m_invitationInterest.clear();
}

} // namespace chronochat

#if WAF
#include "invitation-request-dialog.moc"
// #include "invitation-request-dialog.cpp.moc"
#endif
