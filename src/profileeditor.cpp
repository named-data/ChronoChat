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

ProfileEditor::ProfileEditor(Ptr<ContactManager> contactManager, 
                             QWidget *parent) 
    : QDialog(parent)
    , ui(new Ui::ProfileEditor)
    , m_tableModel(new QSqlTableModel())
    , m_contactManager(contactManager)
{
  ui->setupUi(this);
  
  Name defaultIdentity = contactManager->getDefaultIdentity();
  ui->identityInput->setText(defaultIdentity.toUri().c_str());

  connect(ui->addRowButton, SIGNAL(clicked()),
          this, SLOT(onAddClicked()));
  connect(ui->deleteRowButton, SIGNAL(clicked()),
          this, SLOT(onDeleteClicked()));
  connect(ui->okButton, SIGNAL(clicked()),
          this, SLOT(onOkClicked()));
  connect(ui->getButton, SIGNAL(clicked()),
          this, SLOT(onGetClicked()));



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
  QSqlRecord record;
  QSqlField identityField("profile_identity", QVariant::String);
  record.append(identityField);
  record.setValue("profile_identity", QString(m_currentIdentity.toUri().c_str()));
  m_tableModel->insertRow(rowCount);
  m_tableModel->setRecord(rowCount, record);
}

void
ProfileEditor::onDeleteClicked()
{
  QItemSelectionModel* selectionModel = ui->profileTable->selectionModel();
  QModelIndexList indexList = selectionModel->selectedRows();

  int i = indexList.size() - 1;  
  for(; i >= 0; i--)
    m_tableModel->removeRow(indexList[i].row());
    
  m_tableModel->submitAll();
}

void
ProfileEditor::onOkClicked()
{
  m_tableModel->submitAll();
  m_contactManager->updateProfileData(m_currentIdentity);
  this->hide();
}

void
ProfileEditor::onGetClicked()
{
  QString inputIdentity = ui->identityInput->text();
  m_currentIdentity = Name(inputIdentity.toUtf8().constData());
  string filter("profile_identity = '");
  filter.append(m_currentIdentity.toUri()).append("'");
  _LOG_DEBUG("filter: " << filter);

  m_tableModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
  m_tableModel->setTable("SelfProfile");
  m_tableModel->setFilter(filter.c_str());
  m_tableModel->select();
  m_tableModel->setHeaderData(0, Qt::Horizontal, QObject::tr("Identity"));
  m_tableModel->setHeaderData(1, Qt::Horizontal, QObject::tr("Type"));
  m_tableModel->setHeaderData(2, Qt::Horizontal, QObject::tr("Value"));

  _LOG_DEBUG("row count: " << m_tableModel->rowCount());

  ui->profileTable->setModel(m_tableModel);
  ui->profileTable->setColumnHidden(0, true);
  ui->profileTable->show();
}

#if WAF
#include "profileeditor.moc"
#include "profileeditor.cpp.moc"
#endif
