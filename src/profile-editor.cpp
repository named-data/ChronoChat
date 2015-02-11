/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "profile-editor.hpp"
#include "ui_profile-editor.h"
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlField>
#include <QtSql/QSqlError>

#ifndef Q_MOC_RUN
#include "logging.h"
#endif

// INIT_LOGGER("ProfileEditor")

namespace chronochat {

ProfileEditor::ProfileEditor(QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::ProfileEditor)
  , m_tableModel(new QSqlTableModel())
{
  ui->setupUi(this);

  connect(ui->addRowButton, SIGNAL(clicked()),
          this, SLOT(onAddClicked()));
  connect(ui->deleteRowButton, SIGNAL(clicked()),
          this, SLOT(onDeleteClicked()));
  connect(ui->okButton, SIGNAL(clicked()),
          this, SLOT(onOkClicked()));
}

ProfileEditor::~ProfileEditor()
{
    delete ui;
    delete m_tableModel;
}

void
ProfileEditor::onCloseDBModule()
{
  // _LOG_DEBUG("close db module");
  if (m_tableModel) {
    delete m_tableModel;
    // _LOG_DEBUG("tableModel closed");
  }
}

void
ProfileEditor::onIdentityUpdated(const QString& identity)
{
  m_tableModel = new QSqlTableModel();

  m_identity = identity;
  ui->identityInput->setText(identity);

  m_tableModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
  m_tableModel->setTable("SelfProfile");
  m_tableModel->select();
  m_tableModel->setHeaderData(0, Qt::Horizontal, QObject::tr("Type"));
  m_tableModel->setHeaderData(1, Qt::Horizontal, QObject::tr("Value"));

  ui->profileTable->setModel(m_tableModel);
  ui->profileTable->show();
}

void
ProfileEditor::onAddClicked()
{
  int rowCount = m_tableModel->rowCount();
  QSqlRecord record;
  m_tableModel->insertRow(rowCount);
  m_tableModel->setRecord(rowCount, record);
}

void
ProfileEditor::onDeleteClicked()
{
  QItemSelectionModel* selectionModel = ui->profileTable->selectionModel();
  QModelIndexList indexList = selectionModel->selectedIndexes();

  for (int i = indexList.size() - 1; i >= 0; i--)
    m_tableModel->removeRow(indexList[i].row());

  m_tableModel->submitAll();
}

void
ProfileEditor::onOkClicked()
{
  m_tableModel->submitAll();
  emit updateProfile();
  this->hide();
}

} // namespace chronochat

#if WAF
#include "profile-editor.moc"
// #include "profile-editor.cpp.moc"
#endif
