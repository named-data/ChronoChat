/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "profileeditor.h"
#include "ui_profileeditor.h"
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlField>
#include <QtSql/QSqlError>

#ifndef Q_MOC_RUN
#include "logging.h"
#include "exception.h"
#endif

using namespace ndn;

INIT_LOGGER("ProfileEditor");

ProfileEditor::ProfileEditor(Ptr<ContactStorage> contactStorage, QWidget *parent) 
    : QDialog(parent)
    , ui(new Ui::ProfileEditor)
    , m_tableModel(new QSqlTableModel())
    , m_contactStorage(contactStorage)
{
    ui->setupUi(this);

    connect(ui->addRowButton, SIGNAL(clicked()),
            this, SLOT(onAddClicked()));
    connect(ui->deleteRowButton, SIGNAL(clicked()),
            this, SLOT(onDeleteClicked()));
    connect(ui->okButton, SIGNAL(clicked()),
            this, SLOT(onOkClicked()));
    
    m_tableModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_tableModel->setTable("SelfProfile");
    m_tableModel->select();
    m_tableModel->setHeaderData(0, Qt::Horizontal, QObject::tr("Index"));
    m_tableModel->setHeaderData(1, Qt::Horizontal, QObject::tr("Type"));
    m_tableModel->setHeaderData(2, Qt::Horizontal, QObject::tr("Value"));

    ui->profileTable->setModel(m_tableModel);
    ui->profileTable->setColumnHidden(0, true);
    ui->profileTable->show();

}

ProfileEditor::~ProfileEditor()
{
    delete ui;
    delete m_tableModel;
}

void
ProfileEditor::onAddClicked()
{
  int rowCount = m_tableModel->rowCount();

  // QSqlRecord record;
  // QSqlField typeField("profile_type", QVariant::String);
  // QSqlField valueField("profile_value", QVariant::String);
  // record.append(typeField);
  // record.append(valueField);
  // record.setValue("profile_type", QString("N/A"));
  // record.setValue("profile_value", QString("N/A"));

  // bool res = m_tableModel->insertRecord(-1, record);

  // res = m_tableModel->submitAll();
  m_tableModel->insertRow(rowCount);
}

void
ProfileEditor::onDeleteClicked()
{
  QItemSelectionModel* selectionModel = ui->profileTable->selectionModel();
  QModelIndexList indexList = selectionModel->selectedRows();

  int i = indexList.size() - 1;  
  for(; i >= 0; i--)
    {
      if(0 != indexList[i].row())
        m_tableModel->removeRow(indexList[i].row());
    }
  m_tableModel->submitAll();
}

void
ProfileEditor::onOkClicked()
{
  m_tableModel->submitAll();
  this->hide();
}

#if WAF
#include "profileeditor.moc"
#include "profileeditor.cpp.moc"
#endif
