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
#include <ndn.cxx/data.h>
#include <ndn.cxx/security/certificate/identity-certificate.h>
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
  setMsg(const std::string& inviter, const std::string& chatroom);

  inline void
  setInterestName(const ndn::Name& interestName)
  { m_interestName = interestName; }

  inline void
  setIdentityCertificate(const ndn::Ptr<ndn::security::IdentityCertificate> identityCertificate)
  { m_identityCertificate = identityCertificate; }

signals:
  void
  invitationAccepted(const ndn::Name& interestName, 
                     const ndn::security::IdentityCertificate& identityCertificate, 
                     QString inviter, 
                     QString chatroom);
  
  void
  invitationRejected(const ndn::Name& interestName);

private slots:
  void
  onOkClicked();
  
  void
  onCancelClicked();


private:
  Ui::InvitationDialog *ui;
  std::string m_inviter;
  std::string m_chatroom;
  ndn::Name m_interestName;
  ndn::Ptr<ndn::security::IdentityCertificate> m_identityCertificate;
};

#endif // INVITATIONDIALOG_H
