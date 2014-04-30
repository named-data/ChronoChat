/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "start-chat-dialog.h"
#include "ui_start-chat-dialog.h"

StartChatDialog::StartChatDialog(QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::StartChatDialog)
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
StartChatDialog::setChatroom(const std::string& chatroom)
{
  ui->chatroomInput->setText(QString::fromStdString(chatroom));
}

void
StartChatDialog::onOkClicked()
{
  QString chatroom = ui->chatroomInput->text();
  bool secured = ui->withSecurity->isChecked();
  emit startChatroom(chatroom, secured);
  this->close();
}

void
StartChatDialog::onCancelClicked()
{
  this->close();
}

#if WAF
#include "start-chat-dialog.moc"
#include "start-chat-dialog.cpp.moc"
#endif
