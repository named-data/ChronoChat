/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "invite-list-dialog.h"
#include "ui_invite-list-dialog.h"

InviteListDialog::InviteListDialog(QWidget *parent)
  :QDialog(parent)
  , ui(new Ui::InviteListDialog)
  , m_contactListModel(new QStringListModel)
{
  ui->setupUi(this);

  ui->contactListView->setModel(m_contactListModel);

  connect(ui->inviteButton, SIGNAL(clicked()),
          this, SLOT(onInviteClicked()));
  connect(ui->cancelButton, SIGNAL(clicked()),
          this, SLOT(onCancelClicked()));
}

InviteListDialog::~InviteListDialog()
{
  delete ui;
  delete m_contactListModel;
}

void
InviteListDialog::setInviteLabel(std::string label)
{
  std::string msg("invite to chatroom:\n");
  msg += label;
  ui->inviteLabel->setText(QString::fromStdString(msg));
}

void
InviteListDialog::onInviteClicked()
{
  QModelIndexList selected = ui->contactListView->selectionModel()->selectedIndexes();
  QString alias = m_contactListModel->data(selected.first(), Qt::DisplayRole).toString();

  for(int i = 0; i < m_contactAliasList.size(); i++) // TODO:: could be optimized without using for loop.
    {
      if(alias == m_contactAliasList[i])
        {
          emit sendInvitation(m_contactIdList[i]);
          break;
        }
    }

  this->close();
}

void
InviteListDialog::onCancelClicked()
{
  this->close();
}

void
InviteListDialog::onContactAliasListReady(const QStringList& aliasList)
{
  m_contactAliasList = aliasList;
  m_contactListModel->setStringList(m_contactAliasList);
}

void
InviteListDialog::onContactIdListReady(const QStringList& idList)
{
  m_contactIdList = idList;
}

#if WAF
#include "invite-list-dialog.moc"
#include "invite-list-dialog.cpp.moc"
#endif
