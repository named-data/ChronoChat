/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "chronochat.h"
#include "ui_chronochat.h"

ChronoChat::ChronoChat(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChronoChat)
{
    ui->setupUi(this);
}

ChronoChat::~ChronoChat()
{
    delete ui;
}

#if WAF
#include "chronochat.moc"
#include "chronochat.cpp.moc"
#endif
