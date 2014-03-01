/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */


#include "browse-contact-dialog.h"
#include "ui_browse-contact-dialog.h"

#ifndef Q_MOC_RUN
#include "profile.h"
#endif

using namespace std;
using namespace ndn;
using namespace chronos;

BrowseContactDialog::BrowseContactDialog(QWidget *parent) 
  : QDialog(parent)
  , ui(new Ui::BrowseContactDialog)
  , m_contactListModel(new QStringListModel)
{
  ui->setupUi(this);

  m_typeHeader = new QTableWidgetItem(QString("Type"));
  m_valueHeader = new QTableWidgetItem(QString("Value"));
  ui->InfoTable->setHorizontalHeaderItem(0, m_typeHeader);
  ui->InfoTable->setHorizontalHeaderItem(1, m_valueHeader);

  ui->ContactList->setModel(m_contactListModel);

  connect(ui->ContactList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          this, SLOT(onSelectionChanged(const QItemSelection &, const QItemSelection &)));
  connect(ui->AddButton, SIGNAL(clicked()),
	  this, SLOT(onAddClicked()));
  connect(ui->DirectAddButton, SIGNAL(clicked()),
	  this, SLOT(onDirectAddClicked()));
}

BrowseContactDialog::~BrowseContactDialog()
{
  delete m_typeHeader;
  delete m_valueHeader;
  delete ui;
}

void
BrowseContactDialog::onSelectionChanged(const QItemSelection& selected,
                                        const QItemSelection& deselected)
{
  QModelIndexList items = selected.indexes();
  QString certName = m_contactCertNameList[items.first().row()];
  emit fetchIdCert(certName);
}

void
BrowseContactDialog::onAddClicked()
{
  QItemSelectionModel* selectionModel = ui->ContactList->selectionModel();
  QModelIndexList selectedList = selectionModel->selectedIndexes();
  QModelIndexList::iterator it = selectedList.begin();
  for(; it != selectedList.end(); it++)
    emit addContact(m_contactNameList[it->row()]);

  this->close();
}

void
BrowseContactDialog::onDirectAddClicked()
{
  emit directAddClicked();
  this->close();
}

void
BrowseContactDialog::closeEvent(QCloseEvent *e)
{
  ui->InfoTable->clear();
  for(int i = ui->InfoTable->rowCount() - 1; i >= 0 ; i--)
      ui->InfoTable->removeRow(i);
  ui->InfoTable->horizontalHeader()->hide();

  hide();
  e->ignore();
}

void
BrowseContactDialog::onIdCertNameListReady(const QStringList& qCertNameList)
{
  m_contactCertNameList = qCertNameList;
}

void
BrowseContactDialog::onNameListReady(const QStringList& qNameList)
{
  m_contactNameList = qNameList;
  m_contactListModel->setStringList(m_contactNameList);
}

void
BrowseContactDialog::onIdCertReady(const IdentityCertificate& idCert)
{
  Profile profile(idCert);

  ui->InfoTable->clear();

  for(int i = ui->InfoTable->rowCount() - 1; i >= 0 ; i--)
    ui->InfoTable->removeRow(i);

  ui->InfoTable->horizontalHeader()->show();
  ui->InfoTable->setColumnCount(2);
  
  Profile::const_iterator proIt = profile.begin();
  Profile::const_iterator proEnd = profile.end();
  int rowCount = 0;

  for(; proIt != proEnd; proIt++, rowCount++)
    {
      ui->InfoTable->insertRow(rowCount);  
      QTableWidgetItem* type = new QTableWidgetItem(QString::fromStdString(proIt->first));
      ui->InfoTable->setItem(rowCount, 0, type);
      
      QTableWidgetItem* value = new QTableWidgetItem(QString::fromStdString(proIt->second));
      ui->InfoTable->setItem(rowCount, 1, value);	  
    }
}

#if WAF
#include "browse-contact-dialog.moc"
#include "browse-contact-dialog.cpp.moc"
#endif
