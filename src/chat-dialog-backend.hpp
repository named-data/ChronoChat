/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2020, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 *         Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_CHAT_DIALOG_BACKEND_HPP
#define CHRONOCHAT_CHAT_DIALOG_BACKEND_HPP

#include <QThread>

#ifndef Q_MOC_RUN
#include "common.hpp"
#include "chatroom-info.hpp"
#include "chat-message.hpp"
#include <mutex>
#include <ChronoSync/socket.hpp>
#include <boost/thread.hpp>
#include <ndn-cxx/security/validator-config.hpp>
#endif

namespace chronochat {

class NodeInfo
{
public:
  QString sessionPrefix;
  chronosync::SeqNo seqNo;
};

class UserInfo
{
public:
  ndn::Name sessionPrefix;
  bool hasNick;
  std::string userNick;
  ndn::scheduler::ScopedEventId timeoutEventId;
};

class ChatDialogBackend : public QThread
{
  Q_OBJECT

public:
  ChatDialogBackend(const Name& chatroomPrefix,
                    const Name& userChatPrefix,
                    const Name& routingPrefix,
                    const std::string& chatroomName,
                    const std::string& nick,
                    const Name& signingId = Name(),
                    QObject* parent = nullptr);

  ~ChatDialogBackend();

protected:
  void
  run();

private:
  void
  initializeSync();

  void
  exitChatroom();

  void
  close();

  void
  processSyncUpdate(const std::vector<chronosync::MissingDataInfo>& updates);

  void
  processChatData(const ndn::Data& data,
                  bool needDisplay,
                  bool isValidated);

  void
  remoteSessionTimeout(const Name& sessionPrefix);

  void
  sendMsg(ChatMessage& msg);

  void
  sendJoin();

  void
  sendHello();

  void
  sendLeave();

  void
  prepareControlMessage(ChatMessage& msg,
                        ChatMessage::ChatMessageType type);

  void
  prepareChatMessage(const QString& text,
                     time_t timestamp,
                     ChatMessage& msg);

  void
  updatePrefixes();

signals:
  void
  syncTreeUpdated(std::vector<chronochat::NodeInfo> updates, QString digest);

  void
  chatMessageReceived(QString nick, QString text, time_t timestamp);

  void
  sessionRemoved(QString sessionPrefix, QString nick, time_t timestamp);

  void
  messageReceived(QString sessionPrefix, QString nick, uint64_t seqNo, time_t timestamp,
                  bool addSession);

  void
  chatPrefixChanged(ndn::Name newChatPrefix);

  void
  refreshChatDialog(ndn::Name chatPrefix);

  void
  eraseInRoster(ndn::Name sessionPrefix, ndn::Name::Component chatroomName);

  void
  addInRoster(ndn::Name sessionPrefix, ndn::Name::Component chatroomName);

  void
  newChatroomForDiscovery(ndn::Name::Component chatroomName);

  void
  nfdError();

public slots:
  void
  sendChatMessage(QString text, time_t timestamp);

  void
  updateRoutingPrefix(const QString& localRoutingPrefix);

  void
  shutdown();

  void
  onNfdReconnect();

private:
  typedef std::map<ndn::Name, UserInfo> BackendRoster;

  bool m_shouldResume;
  bool m_isNfdConnected;
  shared_ptr<ndn::Face> m_face;

  Name m_localRoutingPrefix;                                    // routable local prefix
  Name m_chatroomPrefix;                                        // chatroom sync prefix
  Name m_userChatPrefix;                                        // user chat prefix
  Name m_routableUserChatPrefix;                                // routable user chat prefix

  std::string m_chatroomName;                                   // chatroom name
  std::string m_nick;                                           // user nick

  Name m_signingId;                                             // signing identity
  shared_ptr<ndn::security::ValidatorConfig> m_validator;       // validator
  shared_ptr<chronosync::Socket> m_sock;                        // SyncSocket

  unique_ptr<ndn::Scheduler> m_scheduler;                       // scheduler
  ndn::scheduler::EventId m_helloEventId;                       // event id of timeout

  bool m_joined;                                                // true if in a chatroom

  BackendRoster m_roster;                                       // User roster

  std::mutex m_resumeMutex;
  std::mutex m_nfdConnectionMutex;
};

} // namespace chronochat

#endif // CHRONOCHAT_CHAT_DIALOG_BACKEND_HPP
