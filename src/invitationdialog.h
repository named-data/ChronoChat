/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef INVITATIONDIALOG_H
#define INVITATIONDIALOG_H

#include <QDialog>

namespace Ui {
class InvitationDialog;
}

class InvitationDialog : public QDialog
{
    Q_OBJECT

public:
  explicit InvitationDialog(QWidget *parent = 0);
  ~InvitationDialog();

  void
  setMsg(const std::string& inviter, const std::string& chatroom);

  inline void
  setInterestName(const ndn::Name& interestName)
  { m_interestName = interestName; }

signals:
  void
  invitationAccepted(const ndn::Name& interestName);
  
  void
  invitationRejected(const ndn::Name& interestName);

private slots:
  void
  onOkClicked();
  
  void
  onCancelClicked();


private:
  Ui::InvitationDialog *ui;
  ndn::Name m_interestName;
};

#endif // INVITATIONDIALOG_H
