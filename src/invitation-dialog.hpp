/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOS_INVITATION_DIALOG_HPP
#define CHRONOS_INVITATION_DIALOG_HPP

#include <QDialog>

#ifndef Q_MOC_RUN
#include "common.hpp"
#endif

namespace Ui {
class InvitationDialog;
}

namespace chronos{

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

} // namespace chronos

#endif // CHRONOS_INVITATION_DIALOG_HPP
