/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef INVITATION_DIALOG_H
#define INVITATION_DIALOG_H

#include <QDialog>

#ifndef Q_MOC_RUN
#include <ndn-cpp-dev/name.hpp>
#endif

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
  setInvitation(const std::string& alias,
                const std::string& chatroom,
                const ndn::Name& invitationInterest);

signals:
  void
  invitationResponded(const ndn::Name& invitationName, bool accepted);

private slots:
  void
  onOkClicked();
  
  void
  onCancelClicked();


private:
  Ui::InvitationDialog *ui;
  ndn::Name m_invitationInterest;
};

#endif // INVITATION_DIALOG_H
