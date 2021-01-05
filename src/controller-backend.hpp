/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2020, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 *         Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_CONTROLLER_BACKEND_HPP
#define CHRONOCHAT_CONTROLLER_BACKEND_HPP

#include <QString>
#include <QThread>
#include <QStringList>
#include <QMutex>

#ifndef Q_MOC_RUN
#include "common.hpp"
#include "contact-manager.hpp"
#include "invitation.hpp"
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/ims/in-memory-storage-persistent.hpp>
#include <ndn-cxx/security/validator-null.hpp>
#include <ndn-cxx/face.hpp>
#include <boost/thread.hpp>
#include <mutex>
#endif

namespace chronochat {

class ControllerBackend : public QThread
{
  Q_OBJECT

public:
  ControllerBackend(QObject* parent = nullptr);

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
  onInvitationInterest(const ndn::Name& prefix, const ndn::Interest& interest,
                       size_t routingPrefixOffset);

  void
  onInvitationRequestInterest(const ndn::Name& prefix, const ndn::Interest& interest,
                              size_t routingPrefixOffset);

  void
  onInvitationRegisterFailed(const Name& prefix, const std::string& failInfo);

  void
  onInvitationValidated(const Interest& interest);

  void
  onInvitationValidationFailed(const Interest& interest,
                               const ndn::security::ValidationError& failureInfo);

  void
  onLocalPrefix(const ndn::ConstBufferPtr& data);

  void
  onLocalPrefixError(uint32_t code, const std::string& msg);

  void
  updateLocalPrefix(const Name& localPrefix);

  void
  onRequestResponse(const Interest& interest, const Data& data);

  void
  onRequestTimeout(const Interest& interest, int& resendTimes);

signals:
  void
  identityUpdated(const QString& identity);

  void
  localPrefixUpdated(const QString& localPrefix);

  void
  invitationValidated(QString alias, QString chatroom, ndn::Name invitationINterest);

  void
  invitationRequestReceived(QString alias, QString chatroom, ndn::Name invitationRequestInterest);

  void
  startChatroomOnInvitation(chronochat::Invitation invitation, bool secured);

  void
  startChatroom(const QString& chatroomName, bool secured);

  void
  invitationRequestResult(const std::string& msg);

  void
  nfdError();

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

  void
  onInvitationRequestResponded(const ndn::Name& invitationName, bool accepted);

  void
  onSendInvitationRequest(const QString& chatroomName, const QString& prefix);

  void
  onNfdReconnect();

private slots:
  void
  onContactIdListReady(const QStringList& list);

private:
  bool m_isNfdConnected;
  bool m_shouldResume;

  Name m_identity;  //TODO: set/get

  Name m_localPrefix;

  ndn::KeyChain m_keyChain;
  ndn::Face m_face;

  // Contact Manager
  ContactManager m_contactManager;

  shared_ptr<ndn::security::Validator> m_validator;
  ndn::security::ValidatorNull m_nullValidator;

  // RegisteredPrefixId
  ndn::ScopedRegisteredPrefixHandle m_invitationListenerHandle;
  ndn::ScopedRegisteredPrefixHandle m_requestListenerHandle;

  // ChatRoomList
  QStringList m_chatDialogList;

  QMutex m_mutex;
  std::mutex m_resumeMutex;
  std::mutex m_nfdConnectionMutex;

  ndn::InMemoryStoragePersistent m_ims;
};

} // namespace chronochat

#endif // CHRONOCHAT_CONTROLLER_BACKEND_HPP
