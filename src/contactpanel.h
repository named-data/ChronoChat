/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CONTACTPANEL_H
#define CONTACTPANEL_H

#include <QDialog>
#include <QStringListModel>
#include <QtSql/QSqlDatabase>
#include <QMenu>

#include "profileeditor.h"
#include "addcontactpanel.h"
#include "setaliasdialog.h"
#include "startchatdialog.h"

#ifndef Q_MOC_RUN
#include "contact-manager.h"
#endif

namespace Ui {
class ContactPanel;
}

class ContactPanel : public QDialog
{
    Q_OBJECT

public:
  explicit ContactPanel(ndn::Ptr<ContactManager> contactManager, QWidget *parent = 0);
  ~ContactPanel();

private slots:
  void
  updateSelection(const QItemSelection &selected,
                  const QItemSelection &deselected);

  void
  openProfileEditor();

  void
  openAddContactPanel();

  void
  openSetAliasDialog();
  
  void
  openStartChatDialog();

  void
  refreshContactList();

  void
  showContextMenu(const QPoint& pos);

  void
  startChatroom(const QString& chatroom, const QString& invitee, bool isIntroducer);

private:
  Ui::ContactPanel *ui;
  ndn::Ptr<ContactManager> m_contactManager;
  QStringListModel* m_contactListModel;
  ProfileEditor* m_profileEditor;
  AddContactPanel* m_addContactPanel;
  SetAliasDialog* m_setAliasDialog;
  StartChatDialog* m_startChatDialog;
  QAction* m_menuInvite;
  QAction* m_menuAlias;


  std::vector<ndn::Ptr<ContactItem> > m_contactList;

  std::string m_currentSelectedContactAlias;
  std::string m_currentSelectedContactNamespace;
};

#endif // CONTACTPANEL_H
