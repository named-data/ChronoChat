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


#include <QStringList>
#include <QItemSelectionModel>
#include <QModelIndex>

ContactPanel::ContactPanel(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ContactPanel),
    m_contactListModel(new QStringListModel)
{
    ui->setupUi(this);

    QStringList contactNameList;
    contactNameList << "Alex" << "Wentao" << "Yingdi";

    m_contactListModel->setStringList(contactNameList);
    ui->ContactList->setModel(m_contactListModel);

    QItemSelectionModel* selectionModel = ui->ContactList->selectionModel();
    connect(selectionModel, SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
	    this, SLOT(updateSelection(const QItemSelection &, const QItemSelection &)));
}

ContactPanel::~ContactPanel()
{
    delete ui;
    delete m_contactListModel;
}

void
ContactPanel::updateSelection(const QItemSelection &selected,
			      const QItemSelection &deselected)
{
  QModelIndexList items = selected.indexes();
  QString text = m_contactListModel->data(items.first(), Qt::DisplayRole).toString();
  ui->NameData->setText(text);
}

#if WAF
#include "contactpanel.moc"
#include "contactpanel.cpp.moc"
#endif
