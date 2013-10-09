/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "contactpanel.h"
#include "ui_contactpanel.h"

ContactPanel::ContactPanel(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ContactPanel)
{
    ui->setupUi(this);
}

ContactPanel::~ContactPanel()
{
    delete ui;
}


#if WAF
#include "contactpanel.moc"
#include "contactpanel.cpp.moc"
#endif
