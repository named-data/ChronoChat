/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_INVITATION_DIALOG_HPP
#define CHRONOCHAT_INVITATION_DIALOG_HPP

#include <QDialog>

#ifndef Q_MOC_RUN
#include "common.hpp"
#endif

namespace Ui {
class InvitationDialog;
}

namespace chronochat {

class InvitationDialog : public QDialog
{
    Q_OBJECT

public:
  explicit
  InvitationDialog(QWidget* parent = 0);

  ~InvitationDialog();

signals:
  void
  invitationResponded(const ndn::Name& invitationName, bool accepted);

public slots:
  void
  onInvitationReceived(QString alias, QString chatroom, ndn::Name invitationInterest);

private slots:
  void
  onOkClicked();

  void
  onCancelClicked();


private:
  Ui::InvitationDialog* ui;
  ndn::Name m_invitationInterest;
};

} // namespace chronochat

#endif // CHRONOCHAT_INVITATION_DIALOG_HPP
