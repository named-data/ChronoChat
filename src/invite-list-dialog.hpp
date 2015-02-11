/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_INVITE_LIST_DIALOG_HPP
#define CHRONOCHAT_INVITE_LIST_DIALOG_HPP

#include <QDialog>
#include <QStringListModel>

#ifndef Q_MOC_RUN
#endif

namespace Ui {
class InviteListDialog;
}

namespace chronochat {

class InviteListDialog : public QDialog
{
  Q_OBJECT

public:
  explicit
  InviteListDialog(QWidget* parent = 0);

  ~InviteListDialog();

  void
  setInviteLabel(std::string label);

signals:
  void
  sendInvitation(const QString&);

public slots:
  void
  onContactAliasListReady(const QStringList& aliasList);

  void
  onContactIdListReady(const QStringList& idList);

private slots:
  void
  onInviteClicked();

  void
  onCancelClicked();

private:
  Ui::InviteListDialog* ui;
  QStringListModel* m_contactListModel;
  QStringList m_contactAliasList;
  QStringList m_contactIdList;
};

} // namespace chronochat

#endif // CHRONOCHAT_INVITE_LIST_DIALOG_HPP
