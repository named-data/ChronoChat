/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "add-contact-panel.h"
#include "ui_add-contact-panel.h"

#ifndef Q_MOC_RUN
#endif

AddContactPanel::AddContactPanel(QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::AddContactPanel)
{
  ui->setupUi(this);

  connect(ui->cancelButton, SIGNAL(clicked()),
          this, SLOT(onCancelClicked()));
  connect(ui->searchButton, SIGNAL(clicked()),
          this, SLOT(onSearchClicked()));
  connect(ui->addButton, SIGNAL(clicked()),
          this, SLOT(onAddClicked()));

  ui->infoView->setColumnCount(3);

  m_typeHeader = new QTableWidgetItem(QString("Type"));
  ui->infoView->setHorizontalHeaderItem(0, m_typeHeader);
  m_valueHeader = new QTableWidgetItem(QString("Value"));
  ui->infoView->setHorizontalHeaderItem(1, m_valueHeader);
  m_endorseHeader = new QTableWidgetItem(QString("Endorse"));
  ui->infoView->setHorizontalHeaderItem(2, m_endorseHeader);

}

AddContactPanel::~AddContactPanel()
{
  delete m_typeHeader;
  delete m_valueHeader;
  delete m_endorseHeader;

  delete ui;
}

void
AddContactPanel::onCancelClicked()
{
  this->close(); 
}

void
AddContactPanel::onSearchClicked()
{
  // ui->infoView->clear();
  for(int i = ui->infoView->rowCount() - 1; i >= 0 ; i--)
    ui->infoView->removeRow(i);

  m_searchIdentity = ui->contactInput->text();
  emit fetchInfo(m_searchIdentity);
}

void
AddContactPanel::onAddClicked()
{
  emit addContact(m_searchIdentity);
  this->close();
}

void
AddContactPanel::onContactEndorseInfoReady(const chronos::EndorseInfo& endorseInfo)
{
  int entrySize = endorseInfo.endorsement_size();

  for(int rowCount = 0; rowCount < entrySize; rowCount++)
    {
      ui->infoView->insertRow(rowCount);  
      QTableWidgetItem* type = new QTableWidgetItem(QString::fromStdString(endorseInfo.endorsement(rowCount).type()));
      ui->infoView->setItem(rowCount, 0, type);
    
      QTableWidgetItem* value = new QTableWidgetItem(QString::fromStdString(endorseInfo.endorsement(rowCount).value()));
      ui->infoView->setItem(rowCount, 1, value);

      QTableWidgetItem* endorse = new QTableWidgetItem(QString::fromStdString(endorseInfo.endorsement(rowCount).endorse()));
      ui->infoView->setItem(rowCount, 2, endorse);
    }
}


#if WAF
#include "add-contact-panel.moc"
#include "add-contact-panel.cpp.moc"
#endif
