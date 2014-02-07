/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef INVITELISTDIALOG_H
#define INVITELISTDIALOG_H

#include <QDialog>
#include <QStringListModel>

#ifndef Q_MOC_RUN
#include "contact-manager.h"
#endif

namespace Ui {
class InviteListDialog;
}

class InviteListDialog : public QDialog
{
  Q_OBJECT

public:
  explicit InviteListDialog(ndn::shared_ptr<chronos::ContactManager> contactManager,
                            QWidget *parent = 0);
  ~InviteListDialog();

  void
  setInviteLabel(std::string label);
  
signals:
  void
  invitionDetermined(QString, bool);

private slots:
  void
  refreshContactList();

  void 
  inviteWrapper();

  void
  onCancelClicked();

private:
  Ui::InviteListDialog *ui;
  ndn::shared_ptr<chronos::ContactManager> m_contactManager;
  QStringListModel* m_contactListModel;
  std::vector<ndn::shared_ptr<chronos::ContactItem> > m_contactList;
  std::vector<std::string> m_invitedContacts;
};

#endif // INVITELISTDIALOG_H
