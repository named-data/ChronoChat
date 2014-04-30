/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "contact-panel.h"
#include "ui_contact-panel.h"

#include <QMenu>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlField>
#include <QtSql/QSqlError>

#ifndef Q_MOC_RUN
#include "logging.h"
#endif

INIT_LOGGER("ContactPanel");

ContactPanel::ContactPanel(QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::ContactPanel)
  , m_setAliasDialog(new SetAliasDialog)
  , m_contactListModel(new QStringListModel)
  , m_trustScopeModel(0)
  , m_endorseDataModel(0)
  , m_endorseComboBoxDelegate(new EndorseComboBoxDelegate)
{
  ui->setupUi(this);
  ui->ContactList->setModel(m_contactListModel);
  m_menuAlias  = new QAction("Set Alias", this);
  m_menuDelete = new QAction("Delete", this);

  connect(ui->ContactList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          this, SLOT(onSelectionChanged(const QItemSelection &, const QItemSelection &)));
  connect(ui->ContactList, SIGNAL(customContextMenuRequested(const QPoint&)),
          this, SLOT(onContextMenuRequested(const QPoint&)));
  connect(ui->isIntroducer, SIGNAL(stateChanged(int)),
          this, SLOT(onIsIntroducerChanged(int)));
  connect(ui->addScope, SIGNAL(clicked()),
          this, SLOT(onAddScopeClicked()));
  connect(ui->deleteScope, SIGNAL(clicked()),
          this, SLOT(onDeleteScopeClicked()));
  connect(ui->saveButton, SIGNAL(clicked()),
          this, SLOT(onSaveScopeClicked()));
  connect(ui->endorseButton, SIGNAL(clicked()),
          this, SLOT(onEndorseButtonClicked()));
  connect(m_setAliasDialog, SIGNAL(aliasChanged(const QString&, const QString&)),
          this, SLOT(onAliasChanged(const QString&, const QString&)));
}

ContactPanel::~ContactPanel()
{
  if(m_contactListModel) delete m_contactListModel;
  if(m_trustScopeModel)  delete m_trustScopeModel;
  if(m_endorseDataModel) delete m_endorseDataModel;

  delete m_setAliasDialog;
  delete m_endorseComboBoxDelegate;

  delete m_menuAlias;
  delete m_menuDelete;

  delete ui;
}

//private methods
void
ContactPanel::resetPanel()
{
  // Clean up General tag.
  ui->NameData->clear();
  ui->NameSpaceData->clear();
  ui->InstitutionData->clear();

  // Clean up TrustScope tag.
  ui->isIntroducer->setChecked(false);
  ui->addScope->setEnabled(false);
  ui->deleteScope->setEnabled(false);

  m_trustScopeModel = new QSqlTableModel;
  m_trustScopeModel->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
  m_trustScopeModel->setHeaderData(1, Qt::Horizontal, QObject::tr("Contact"));
  m_trustScopeModel->setHeaderData(2, Qt::Horizontal, QObject::tr("TrustScope"));

  ui->trustScopeList->setModel(m_trustScopeModel);
  ui->trustScopeList->setColumnHidden(0, true);
  ui->trustScopeList->setColumnHidden(1, true);
  ui->trustScopeList->show();
  ui->trustScopeList->setEnabled(false);

  // Clean up Endorse tag.
  m_endorseDataModel = new QSqlTableModel;
  m_endorseDataModel->setHeaderData(0, Qt::Horizontal, QObject::tr("Identity"));
  m_endorseDataModel->setHeaderData(1, Qt::Horizontal, QObject::tr("Type"));
  m_endorseDataModel->setHeaderData(2, Qt::Horizontal, QObject::tr("Value"));
  m_endorseDataModel->setHeaderData(3, Qt::Horizontal, QObject::tr("Endorse"));

  ui->endorseList->setModel(m_endorseDataModel);
  ui->endorseList->setColumnHidden(0, true);
  ui->endorseList->resizeColumnToContents(1);
  ui->endorseList->resizeColumnToContents(2);
  ui->endorseList->setItemDelegateForColumn(3, m_endorseComboBoxDelegate);
  ui->endorseList->show();
  ui->endorseList->setEnabled(false);

  // Clean up contact list.
  m_contactAliasList.clear();
  m_contactIdList.clear();
  m_currentSelectedContact.clear();
  m_contactListModel->setStringList(m_contactAliasList);
}

// public slots
void
ContactPanel::onCloseDBModule()
{
  _LOG_DEBUG("close db module");
  if(m_trustScopeModel)
    {
      delete m_trustScopeModel;
      _LOG_DEBUG("trustScopeModel closed");
    }
  if(m_endorseDataModel)
    {
      delete m_endorseDataModel;
      _LOG_DEBUG("endorseDataModel closed");
    }
}

void
ContactPanel::onIdentityUpdated(const QString& identity)
{
  resetPanel();

  emit waitForContactList();  // Re-load contact list;
}

void
ContactPanel::onContactAliasListReady(const QStringList& aliasList)
{
  m_contactAliasList = aliasList;
  m_contactListModel->setStringList(m_contactAliasList);
}

void
ContactPanel::onContactIdListReady(const QStringList& idList)
{
  m_currentSelectedContact.clear();
  m_contactIdList = idList;
}

void
ContactPanel::onContactInfoReady(const QString& identity,
                                 const QString& name,
                                 const QString& institute,
                                 bool isIntro)
{
  ui->NameData->setText(name);
  ui->NameSpaceData->setText(identity);
  ui->InstitutionData->setText(institute);

  QString filter = QString("contact_namespace = '%1'").arg(identity);
  m_trustScopeModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
  m_trustScopeModel->setTable("TrustScope");
  m_trustScopeModel->setFilter(filter);
  m_trustScopeModel->select();
  ui->trustScopeList->setModel(m_trustScopeModel);
  ui->trustScopeList->setColumnHidden(0, true);
  ui->trustScopeList->setColumnHidden(1, true);
  ui->trustScopeList->show();

  if(isIntro)
    {
      ui->isIntroducer->setChecked(true);
      ui->addScope->setEnabled(true);
      ui->deleteScope->setEnabled(true);
      ui->trustScopeList->setEnabled(true);
    }
  else
    {
      ui->isIntroducer->setChecked(false);
      ui->addScope->setEnabled(false);
      ui->deleteScope->setEnabled(false);
      ui->trustScopeList->setEnabled(false);
    }

  QString filter2 = QString("profile_identity = '%1'").arg(identity);
  m_endorseDataModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
  m_endorseDataModel->setTable("ContactProfile");
  m_endorseDataModel->setFilter(filter2);
  m_endorseDataModel->select();
  ui->endorseList->setModel(m_endorseDataModel);
  ui->endorseList->setColumnHidden(0, true);
  ui->endorseList->resizeColumnToContents(1);
  ui->endorseList->resizeColumnToContents(2);
  ui->endorseList->setItemDelegateForColumn(3, m_endorseComboBoxDelegate);
  ui->endorseList->show();
  ui->endorseList->setEnabled(true);
}

// private slots
void
ContactPanel::onSelectionChanged(const QItemSelection &selected,
                                 const QItemSelection &deselected)
{
  QModelIndexList items = selected.indexes();
  QString alias = m_contactListModel->data(items.first(), Qt::DisplayRole).toString();

  bool contactFound = false;
  for(int i = 0; i < m_contactAliasList.size(); i++)
    {
      if(alias == m_contactAliasList[i])
        {
          contactFound = true;
          m_currentSelectedContact = m_contactIdList[i];
          break;
        }
    }

  if(!contactFound)
    {
      emit warning("This should not happen: ContactPanel::updateSelection #1");
      return;
    }

  emit waitForContactInfo(m_currentSelectedContact);
}

void
ContactPanel::onContextMenuRequested(const QPoint& pos)
{
  QMenu menu(ui->ContactList);

  menu.addAction(m_menuAlias);
  connect(m_menuAlias, SIGNAL(triggered()),
          this, SLOT(onSetAliasDialogRequested()));

  menu.addAction(m_menuDelete);
  connect(m_menuDelete, SIGNAL(triggered()),
          this, SLOT(onContactDeletionRequested()));

  menu.exec(ui->ContactList->mapToGlobal(pos));
}

void
ContactPanel::onSetAliasDialogRequested()
{
  for(int i = 0; i < m_contactIdList.size(); i++)
    if(m_contactIdList[i] == m_currentSelectedContact)
      {
        m_setAliasDialog->setTargetIdentity(m_currentSelectedContact, m_contactAliasList[i]);
        m_setAliasDialog->show();
        return;
      }
}

void
ContactPanel::onContactDeletionRequested()
{
  QItemSelectionModel* selectionModel = ui->ContactList->selectionModel();
  QModelIndexList selectedList = selectionModel->selectedIndexes();
  QModelIndexList::iterator it = selectedList.begin();
  for(; it != selectedList.end(); it++)
    {
      QString alias =  m_contactListModel->data(*it, Qt::DisplayRole).toString();
      for(int i = 0; i < m_contactAliasList.size(); i++)
        if(m_contactAliasList[i] == alias)
          {
            emit removeContact(m_contactIdList[i]);
            return;
          }
    }
}

void
ContactPanel::onIsIntroducerChanged(int state)
{
  if(state == Qt::Checked)
    {
      ui->addScope->setEnabled(true);
      ui->deleteScope->setEnabled(true);
      ui->trustScopeList->setEnabled(true);
      emit updateIsIntroducer(m_currentSelectedContact, true);
    }
  else
    {
      ui->addScope->setEnabled(false);
      ui->deleteScope->setEnabled(false);
      ui->trustScopeList->setEnabled(false);
      emit updateIsIntroducer(m_currentSelectedContact, false);
    }
}

void
ContactPanel::onAddScopeClicked()
{
  int rowCount = m_trustScopeModel->rowCount();
  QSqlRecord record;
  QSqlField identityField("contact_namespace", QVariant::String);
  record.append(identityField);
  record.setValue("contact_namespace", m_currentSelectedContact);
  m_trustScopeModel->insertRow(rowCount);
  m_trustScopeModel->setRecord(rowCount, record);
}

void
ContactPanel::onDeleteScopeClicked()
{
  QItemSelectionModel* selectionModel = ui->trustScopeList->selectionModel();
  QModelIndexList indexList = selectionModel->selectedIndexes();

  int i = indexList.size() - 1;
  for(; i >= 0; i--)
    m_trustScopeModel->removeRow(indexList[i].row());

  m_trustScopeModel->submitAll();
}

void
ContactPanel::onSaveScopeClicked()
{
  m_trustScopeModel->submitAll();
}

void
ContactPanel::onEndorseButtonClicked()
{
  m_endorseDataModel->submitAll();
  emit updateEndorseCertificate(m_currentSelectedContact);
}

void
ContactPanel::onAliasChanged(const QString& identity, const QString& alias)
{
  emit updateAlias(identity, alias);
}

#if WAF
#include "contact-panel.moc"
#include "contact-panel.cpp.moc"
#endif
