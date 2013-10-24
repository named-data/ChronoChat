/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>

#ifndef Q_MOC_RUN
#include <ndn.cxx/data.h>
#include <ndn.cxx/security/keychain.h>
#include <ndn.cxx/wrapper/wrapper.h>
#include "chatroom-policy-manager.h"
#include "contact-item.h"
#endif

namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ChatDialog(const ndn::Name& chatroomPrefix,
                      const ndn::Name& localPrefix,
                      const ndn::Name& defaultIdentity,
                      QWidget *parent = 0);
  ~ChatDialog();

  const ndn::Name&
  getChatroomPrefix() const
  { return m_chatroomPrefix; }

  const ndn::Name&
  getLocalPrefix() const
  { return m_localPrefix; }

  void
  sendInvitation(ndn::Ptr<ContactItem> contact);

private:
  void
  setWrapper();
  
  void 
  onInviteReplyVerified(ndn::Ptr<ndn::Data> data, const ndn::Name& identity);

  void 
  onInviteTimeout(ndn::Ptr<ndn::Closure> closure, 
                  ndn::Ptr<ndn::Interest> interest, 
                  const ndn::Name& identity, 
                  int retry);

  void
  invitationRejected(const ndn::Name& identity);
  
  void 
  invitationAccepted(const ndn::Name& identity);

  void
  onUnverified(ndn::Ptr<ndn::Data> data);
    
private:
  Ui::ChatDialog *ui;
  ndn::Name m_chatroomPrefix;
  ndn::Name m_localPrefix;
  ndn::Name m_defaultIdentity;
  ndn::Ptr<ChatroomPolicyManager> m_policyManager;
  ndn::Ptr<ndn::security::IdentityManager> m_identityManager;
  ndn::Ptr<ndn::security::Keychain> m_keychain;
  ndn::Ptr<ndn::Wrapper> m_handler;
};

#endif // ChatDIALOG_H
