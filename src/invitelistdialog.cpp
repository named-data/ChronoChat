/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "invitelistdialog.h"
#include "ui_invitelistdialog.h"

using namespace std;
using namespace chronos;

InviteListDialog::InviteListDialog(ndn::shared_ptr<ContactManager> contactManager,
				   QWidget *parent) 
  :QDialog(parent)
  , ui(new Ui::InviteListDialog)
  , m_contactManager(contactManager)
  , m_contactListModel(new QStringListModel)
{
  ui->setupUi(this);

  refreshContactList();

  ui->contactListView->setModel(m_contactListModel);

  connect(ui->inviteButton, SIGNAL(clicked()),
	  this, SLOT(inviteWrapper()));
  connect(ui->cancelButton, SIGNAL(clicked()),
	  this, SLOT(onCancelClicked()));
}

InviteListDialog::~InviteListDialog()
{
  delete ui;
  delete m_contactListModel;
}

void
InviteListDialog::setInviteLabel(string label)
{ 
  string msg("invite to chatroom:\n");
  msg += label;
  ui->inviteLabel->setText(QString::fromStdString(msg)); 
  refreshContactList();
}

void
InviteListDialog::refreshContactList()
{
  m_contactList.clear();
  m_contactManager->getContactItemList(m_contactList);
  QStringList contactNameList;
  for(int i = 0; i < m_contactList.size(); i++)
    {
      contactNameList << QString::fromStdString(m_contactList[i]->getAlias());
    }

  m_contactListModel->setStringList(contactNameList);
}

void
InviteListDialog::inviteWrapper()
{
  QModelIndexList selected = ui->contactListView->selectionModel()->selectedIndexes();
  QString text = m_contactListModel->data(selected.first(), Qt::DisplayRole).toString();
  string alias = text.toStdString();

  int i = 0;
  for(; i < m_contactList.size(); i++)
    {
      if(alias == m_contactList[i]->getAlias())
        break;
    }

  QString invitedContactNamespace = QString::fromStdString(m_contactList[i]->getNameSpace().toUri());

  bool isIntroducer = true;

  emit invitionDetermined(invitedContactNamespace, isIntroducer);

  this->close();
}

void
InviteListDialog::onCancelClicked()
{ this->close(); }

#if WAF
#include "invitelistdialog.moc"
#include "invitelistdialog.cpp.moc"
#endif
