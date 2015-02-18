/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 *         Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_INVITATION_REQUEST_DIALOG_HPP
#define CHRONOCHAT_INVITATION_REQUEST_DIALOG_HPP

#include <QDialog>

#ifndef Q_MOC_RUN
#include "common.hpp"
#endif

namespace Ui {
class InvitationRequestDialog;
}

namespace chronochat{

class InvitationRequestDialog : public QDialog
{
    Q_OBJECT

public:
  explicit
  InvitationRequestDialog(QWidget* parent = 0);

  ~InvitationRequestDialog();

signals:
  void
  invitationRequestResponded(const ndn::Name& invitationName, bool accepted);

public slots:
  void
  onInvitationRequestReceived(QString alias, QString chatroom, ndn::Name invitationInterest);

private slots:
  void
  onOkClicked();

  void
  onCancelClicked();


private:
  Ui::InvitationRequestDialog* ui;
  ndn::Name m_invitationInterest;
};

} // namespace chronochat

#endif // CHRONOCHAT_INVITATION_REQUEST_DIALOG_HPP
