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

#ifndef Q_MOC_RUN
#include <ndn-cpp-dev/data.hpp>
#include <ndn-cpp-dev/security/identity-certificate.hpp>
#include "invitation.h"
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
                const ndn::Name& invitationInterest);

signals:
  void
  invitationAccepted(const ndn::Name& invitationInterest);
  
  void
  invitationRejected(const ndn::Name& invitationInterest);

private slots:
  void
  onOkClicked();
  
  void
  onCancelClicked();


private:
  Ui::InvitationDialog *ui;
  std::string m_inviterAlias;
  ndn::Name m_invitationInterest;
};

#endif // INVITATIONDIALOG_H
