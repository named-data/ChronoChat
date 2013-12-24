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
using namespace std;
using namespace ndn::ptr_lib;

INIT_LOGGER("ProfileEditor");

ProfileEditor::ProfileEditor(shared_ptr<ContactManager> contactManager, 
                             QWidget *parent) 
    : QDialog(parent)
    , ui(new Ui::ProfileEditor)
    , m_tableModel(new QSqlTableModel())
    , m_contactManager(contactManager)
    , m_identityManager(contactManager->getIdentityManager())
{
  ui->setupUi(this);
  
  m_currentIdentity = contactManager->getDefaultIdentity();
  ui->identityInput->setText(m_currentIdentity.toUri().c_str());

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
  QModelIndexList indexList = selectionModel->selectedIndexes();

  int i = indexList.size() - 1;  
  for(; i >= 0; i--)
    m_tableModel->removeRow(indexList[i].row());
    
  m_tableModel->submitAll();
}

void
ProfileEditor::onOkClicked()
{
  Name defaultCertName = m_identityManager->getDefaultCertificateNameForIdentity(m_currentIdentity);
  if(defaultCertName.size() == 0)
    {
      emit noKeyOrCert(QString::fromStdString("Corresponding certificate is missing!\nHave you installed the certificate?"));
      return;
    }

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

  m_tableModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
  m_tableModel->setTable("SelfProfile");
  m_tableModel->setFilter(filter.c_str());
  m_tableModel->select();
  m_tableModel->setHeaderData(0, Qt::Horizontal, QObject::tr("Identity"));
  m_tableModel->setHeaderData(1, Qt::Horizontal, QObject::tr("Type"));
  m_tableModel->setHeaderData(2, Qt::Horizontal, QObject::tr("Value"));

  ui->profileTable->setModel(m_tableModel);
  ui->profileTable->setColumnHidden(0, true);
  ui->profileTable->show();
}

#if WAF
#include "profileeditor.moc"
#include "profileeditor.cpp.moc"
#endif
