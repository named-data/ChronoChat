/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "warningdialog.h"
#include "ui_warningdialog.h"

using namespace std;

WarningDialog::WarningDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WarningDialog)
{
  ui->setupUi(this);

  connect(ui->okButton, SIGNAL(clicked()),
	  this, SLOT(onOkClicked()));
}

WarningDialog::~WarningDialog()
{
    delete ui;
}

void
WarningDialog::setMsg(const string& msg)
{ ui->message->setText(QString::fromStdString(msg)); }

void
WarningDialog::onOkClicked()
{ this->hide(); }

#if WAF
#include "warningdialog.moc"
#include "warningdialog.cpp.moc"
#endif
