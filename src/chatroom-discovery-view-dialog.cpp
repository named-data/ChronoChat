/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Mengjin Yan <jane.yan0129@gmail.com>
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */
#include "chatroom-discovery-view-dialog.hpp"
#include "ui_chatroom-discovery-view-dialog.h"

namespace chronochat {

using ndn::Name;

ChatroomDiscoveryViewDialog::ChatroomDiscoveryViewDialog(QWidget* parent)
  : QDialog(parent)
  , ui(new Ui::ChatroomDiscoveryViewDialog)
{
  ui->setupUi(this);

  ui->participantsBrowser->setReadOnly(true);

  connect(ui->closeButton, SIGNAL(clicked()),
          this, SLOT(onCloseButtonClicked()));
}

ChatroomDiscoveryViewDialog::~ChatroomDiscoveryViewDialog()
{
  delete ui;
}

void
ChatroomDiscoveryViewDialog::onCloseButtonClicked()
{
  this->close();
}

void
ChatroomDiscoveryViewDialog::setChatroomName(QString chatroomName)
{
  ui->chatroomNameLabel->setText("Chatroom Name: "+chatroomName);
}

void
ChatroomDiscoveryViewDialog::setChatroomTrustModel(QString chatroomTrustModel)
{
  ui->chatroomTrustModelLabel->setText("Chatroom Trust Model: "+chatroomTrustModel);
}

void
ChatroomDiscoveryViewDialog::setChatroomParticipants(const std::list<Name>& participants)
{
  QString content;
  for (std::list<Name>::const_iterator it = participants.begin();
       it != participants.end(); it++) {
    content.append(QString::fromStdString(it->toUri())).append("\n");
  }

  ui->participantsBrowser->setPlainText(content);
}

} // namespace chronochat

#if WAF
#include "chatroom-discovery-view-dialog.moc"
#endif
