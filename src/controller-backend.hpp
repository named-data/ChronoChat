/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_CONTROLLER_BACKEND_HPP
#define CHRONOCHAT_CONTROLLER_BACKEND_HPP

#include <QThread>
#include <QStringList>
#include <QMutex>

#ifndef Q_MOC_RUN
#include "common.hpp"
#include "contact-manager.hpp"
#include "invitation.hpp"
#include "validator-invitation.hpp"
#include <ndn-cxx/security/key-chain.hpp>
#endif

namespace chronochat {

class ControllerBackend : public QThread
{
  Q_OBJECT

public:
  ControllerBackend(QObject* parent = 0);

  ~ControllerBackend();

  ContactManager*
  getContactManager()
  {
    return &m_contactManager;
  }

protected:
  void
  run();

private:
  void
  setInvitationListener();

  ndn::Name
  getInvitationRoutingPrefix();

  void
  onInvitationPrefixReset();

  void
  onInvitationPrefixResetFailed(const std::string& failInfo);

  void
  onInvitationInterest(const ndn::Name& prefix, const ndn::Interest& interest,
                       size_t routingPrefixOffset);

  void
  onInvitationRegisterFailed(const Name& prefix, const std::string& failInfo);

  void
  onInvitationValidated(const shared_ptr<const Interest>& interest);

  void
  onInvitationValidationFailed(const shared_ptr<const Interest>& interest,
                               std::string failureInfo);

  void
  onLocalPrefix(const Interest& interest, Data& data);

  void
  onLocalPrefixTimeout(const Interest& interest);

  void
  updateLocalPrefix(const Name& localPrefix);

signals:
  void
  identityUpdated(const QString& identity);

  void
  localPrefixUpdated(const QString& localPrefix);

  void
  invitaionValidated(QString alias, QString chatroom, ndn::Name invitationINterest);

  void
  startChatroomOnInvitation(chronochat::Invitation invitation, bool secured);

public slots:
  void
  shutdown();

  void
  addChatroom(QString chatroom);

  void
  removeChatroom(QString chatroom);

  void
  onUpdateLocalPrefixAction();

  void
  onIdentityChanged(const QString& identity);

  void
  onInvitationResponded(const ndn::Name& invitationName, bool accepted);

private slots:
  void
  onContactIdListReady(const QStringList& list);

private:
  ndn::Face m_face;

  Name m_identity;  //TODO: set/get

  Name m_localPrefix;

  // Contact Manager
  ContactManager m_contactManager;

  // Security related;
  ndn::KeyChain m_keyChain;
  ValidatorInvitation m_validator;

  // RegisteredPrefixId
  const ndn::RegisteredPrefixId* m_invitationListenerId;

  // ChatRoomList
  QStringList m_chatDialogList;

  QMutex m_mutex;
};

} // namespace chronochat

#endif // CHRONOCHAT_CONTROLLER_BACKEND_HPP
