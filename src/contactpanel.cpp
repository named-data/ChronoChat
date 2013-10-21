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
using namespace std;

INIT_LOGGER("ContactPanel");

ContactPanel::ContactPanel(Ptr<ContactManager> contactManager, QWidget *parent) 
    : QDialog(parent)
    , ui(new Ui::ContactPanel)
    , m_contactManager(contactManager)
    , m_contactListModel(new QStringListModel)
    , m_addContactPanel(new AddContactPanel(contactManager))
    , m_setAliasDialog(new SetAliasDialog(contactManager))
    , m_menuInvite(new QAction("&Invite", this))
    , m_menuAlias(new QAction("&Set Alias", this))
{
  
    ui->setupUi(this);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    QString path = (QDir::home().path());
    path.append(QDir::separator()).append(".chronos").append(QDir::separator()).append("chronos.db");
    db.setDatabaseName(path);
    bool ok = db.open();

    m_profileEditor = new ProfileEditor(m_contactManager);

    refreshContactList();
    ui->ContactList->setModel(m_contactListModel);

    QItemSelectionModel* selectionModel = ui->ContactList->selectionModel();

    connect(selectionModel, SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
	    this, SLOT(updateSelection(const QItemSelection &, const QItemSelection &)));
    connect(ui->ContactList, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(showContextMenu(const QPoint&)));
    connect(ui->EditProfileButton, SIGNAL(clicked()), 
            this, SLOT(openProfileEditor()));

    connect(ui->AddContactButton, SIGNAL(clicked()),
            this, SLOT(openAddContactPanel()));

    connect(m_addContactPanel, SIGNAL(newContactAdded()),
            this, SLOT(refreshContactList()));
    connect(m_setAliasDialog, SIGNAL(aliasChanged()),
            this, SLOT(refreshContactList()));



}

ContactPanel::~ContactPanel()
{
    delete ui;
    delete m_contactListModel;
    delete m_profileEditor;
    delete m_addContactPanel;

    delete m_menuInvite;
}

void
ContactPanel::updateSelection(const QItemSelection &selected,
			      const QItemSelection &deselected)
{
  QModelIndexList items = selected.indexes();
  QString text = m_contactListModel->data(items.first(), Qt::DisplayRole).toString();
  string alias = text.toUtf8().constData();

  m_currentSelectedContact = alias;

  int i = 0;
  for(; i < m_contactList.size(); i++)
    {
      if(alias == m_contactList[i]->getAlias())
        break;
    }
  
  QString name = QString::fromUtf8(m_contactList[i]->getName().c_str());
  QString institution = QString::fromUtf8(m_contactList[i]->getInstitution().c_str());
  QString nameSpace = QString::fromUtf8(m_contactList[i]->getNameSpace().toUri().c_str());
  ui->NameData->setText(name);
  ui->NameSpaceData->setText(nameSpace);
  ui->InstitutionData->setText(institution);
  
  // _LOG_DEBUG("current Alias: " << m_currentSelectedContact);
}

void
ContactPanel::openProfileEditor()
{ m_profileEditor->show(); }

void
ContactPanel::openAddContactPanel()
{ m_addContactPanel->show(); }

void
ContactPanel::refreshContactList()
{
  m_contactList = m_contactManager->getContactItemList();
  QStringList contactNameList;
  for(int i = 0; i < m_contactList.size(); i++)
    contactNameList << QString::fromUtf8(m_contactList[i]->getAlias().c_str());

  m_contactListModel->setStringList(contactNameList);
}

void
ContactPanel::showContextMenu(const QPoint& pos)
{
  QMenu menu(ui->ContactList);
  menu.addAction(m_menuInvite);
  menu.addAction(m_menuAlias);
  connect(m_menuAlias, SIGNAL(triggered()),
          this, SLOT(openSetAliasDialog()));
  menu.exec(ui->ContactList->mapToGlobal(pos));

}

void
ContactPanel::openSetAliasDialog()
{
  m_setAliasDialog->setTargetIdentity(m_currentSelectedContact);
  m_setAliasDialog->show();
}

#if WAF
#include "contactpanel.moc"
#include "contactpanel.cpp.moc"
#endif
