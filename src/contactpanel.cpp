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
#include <QDir>

#ifndef Q_MOC_RUN
#include <boost/filesystem.hpp>
#include "logging.h"
#include "exception.h"
#endif

namespace fs = boost::filesystem;
using namespace ndn;

INIT_LOGGER("ContactPanel");

ContactPanel::ContactPanel(Ptr<ContactStorage> contactStorage, QWidget *parent) 
    : QDialog(parent)
    , ui(new Ui::ContactPanel)
    , m_contactStorage(contactStorage)
    , m_contactListModel(new QStringListModel)
    , m_profileEditor(new ProfileEditor(m_contactStorage))
    , m_addContactPanel(new AddContactPanel())
{
  
    ui->setupUi(this);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    QString path = (QDir::home().path());
    path.append(QDir::separator()).append(".chronos").append(QDir::separator()).append("chronos.db");
    db.setDatabaseName(path);
    bool ok = db.open();

    QStringList contactNameList;
    contactNameList << "Alex" << "Wentao" << "Yingdi";

    m_contactListModel->setStringList(contactNameList);
    ui->ContactList->setModel(m_contactListModel);

    QItemSelectionModel* selectionModel = ui->ContactList->selectionModel();

    connect(selectionModel, SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
	    this, SLOT(updateSelection(const QItemSelection &, const QItemSelection &)));
    connect(ui->EditProfileButton, SIGNAL(clicked()), 
            this, SLOT(openProfileEditor()));

    connect(ui->AddContactButton, SIGNAL(clicked()),
            this, SLOT(openAddContactPanel()));
}

ContactPanel::~ContactPanel()
{
    delete ui;
    delete m_contactListModel;
    delete m_profileEditor;
    delete m_addContactPanel;
}

void
ContactPanel::updateSelection(const QItemSelection &selected,
			      const QItemSelection &deselected)
{
  QModelIndexList items = selected.indexes();
  QString text = m_contactListModel->data(items.first(), Qt::DisplayRole).toString();
  ui->NameData->setText(text);
}

void
ContactPanel::openProfileEditor()
{ m_profileEditor->show(); }

void
ContactPanel::openAddContactPanel()
{ m_addContactPanel->show(); }

#if WAF
#include "contactpanel.moc"
#include "contactpanel.cpp.moc"
#endif
