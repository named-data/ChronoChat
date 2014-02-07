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
{ delete ui; }

void
StartChatDialog::setChatroom(const string& chatroom)
{ ui->chatroomInput->setText(QString::fromStdString(chatroom)); }

void
StartChatDialog::onOkClicked()
{
  QString chatroom = ui->chatroomInput->text();
  emit chatroomConfirmed(chatroom);
  this->close();
}

void
StartChatDialog::onCancelClicked()
{ this->close(); }

#if WAF
#include "startchatdialog.moc"
#include "startchatdialog.cpp.moc"
#endif
