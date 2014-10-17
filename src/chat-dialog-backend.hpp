/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_CHAT_DIALOG_BACKEND_HPP
#define CHRONOCHAT_CHAT_DIALOG_BACKEND_HPP

#include <QThread>
#include <QMutex>

#ifndef Q_MOC_RUN
#include "common.hpp"
#include "chatroom-info.hpp"
#include "chatbuf.pb.h"
#include <socket.hpp>
#endif

namespace chronos {

class NodeInfo {
public:
  QString sessionPrefix;
  chronosync::SeqNo seqNo;
};

class UserInfo {
public:
  ndn::Name sessionPrefix;
  bool hasNick;
  std::string userNick;
  ndn::EventId timeoutEventId;
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
                    QObject* parent = 0);

  ~ChatDialogBackend();

protected:
  void
  run();

private:
  void
  initializeSync();

  void
  processSyncUpdate(const std::vector<chronosync::MissingDataInfo>& updates);

  void
  processChatData(const ndn::shared_ptr<const ndn::Data>& data, bool needDisplay);

  void
  remoteSessionTimeout(const Name& sessionPrefix);

  void
  sendMsg(SyncDemo::ChatMessage& msg);

  void
  sendJoin();

  void
  sendHello();

  void
  sendLeave();

  void
  prepareControlMessage(SyncDemo::ChatMessage& msg,
                        SyncDemo::ChatMessage::ChatMessageType type);

  void
  prepareChatMessage(const QString& text,
                     time_t timestamp,
                     SyncDemo::ChatMessage &msg);

  void
  updatePrefixes();

  std::string
  getHexEncodedDigest(ndn::ConstBufferPtr digest);

signals:
  void
  syncTreeUpdated(std::vector<chronos::NodeInfo> updates, QString digest);

  void
  chatMessageReceived(QString nick, QString text, time_t timestamp);

  void
  sessionAdded(QString sessionPrefix, QString nick, time_t timestamp);

  void
  sessionRemoved(QString sessionPrefix, QString nick, time_t timestamp);

  void
  nickUpdated(QString sessionPrefix, QString nick);

  void
  messageReceived(QString sessionPrefix);

  void
  chatPrefixChanged(ndn::Name newChatPrefix);

public slots:
  void
  sendChatMessage(QString text, time_t timestamp);

  void
  updateRoutingPrefix(const QString& localRoutingPrefix);

  void
  shutdown();

private:
  typedef std::map<ndn::Name, UserInfo> BackendRoster;

  ndn::Face m_face;

  Name m_localRoutingPrefix;             // routable local prefix
  Name m_chatroomPrefix;                 // chatroom sync prefix
  Name m_userChatPrefix;                 // user chat prefix
  Name m_routableUserChatPrefix;         // routable user chat prefix

  std::string m_chatroomName;            // chatroom name
  std::string m_nick;                    // user nick

  shared_ptr<chronosync::Socket> m_sock; // SyncSocket

  ndn::Scheduler m_scheduler;            // scheduler
  ndn::EventId m_helloEventId;           // event id of timeout

  QMutex mutex;                          // mutex used for prefix updates

  bool m_joined;                         // true if in a chatroom

  BackendRoster m_roster;                // User roster
};

} // namespace chronos

#endif // CHRONOCHAT_CHAT_DIALOG_BACKEND_HPP
