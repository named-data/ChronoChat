/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "chatdialog.h"
#include "ui_chatdialog.h"

using namespace std;
using namespace ndn;

ChatDialog::ChatDialog(const Name& chatroomPrefix,
		       const Name& localPrefix,
		       QWidget *parent) 
    : QDialog(parent)
    , m_chatroomPrefix(chatroomPrefix)
    , m_localPrefix(localPrefix)
    , ui(new Ui::ChatDialog)
{
    ui->setupUi(this);
}

ChatDialog::~ChatDialog()
{
    delete ui;
}

void
ChatDialog::sendInvitation()
{
  
}

#if WAF
#include "chatdialog.moc"
#include "chatdialog.cpp.moc"
#endif
