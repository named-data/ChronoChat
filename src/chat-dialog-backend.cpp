/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "chat-dialog-backend.hpp"

#ifndef Q_MOC_RUN
#include <ndn-cxx/util/io.hpp>
#include "logging.h"
#endif


INIT_LOGGER("ChatDialogBackend");

namespace chronos {

static const time::milliseconds FRESHNESS_PERIOD(60000);
static const time::seconds HELLO_INTERVAL(60);
static const uint8_t ROUTING_HINT_SEPARATOR[2] = {0xF0, 0x2E}; // %F0.

ChatDialogBackend::ChatDialogBackend(const Name& chatroomPrefix,
                                     const Name& userChatPrefix,
                                     const Name& routingPrefix,
                                     const std::string& chatroomName,
                                     const std::string& nick,
                                     QObject* parent)
  : QThread(parent)
  , m_localRoutingPrefix(routingPrefix)
  , m_chatroomPrefix(chatroomPrefix)
  , m_userChatPrefix(userChatPrefix)
  , m_chatroomName(chatroomName)
  , m_nick(nick)
  , m_scheduler(m_face.getIoService())
{
  updatePrefixes();
}


ChatDialogBackend::~ChatDialogBackend()
{
}

// protected methods:
void
ChatDialogBackend::run()
{
  initializeSync();

  m_face.processEvents();

  std::cerr << "Bye!" << std::endl;
}

// private methods:
void
ChatDialogBackend::initializeSync()
{
  // if a SyncSocket is running, turn it off
  if (static_cast<bool>(m_sock)) {
    if (m_joined)
      sendLeave();
    m_sock.reset();

    usleep(100000);
  }

  // create a new SyncSocket
  m_sock = make_shared<chronosync::Socket>(m_chatroomPrefix,
                                           m_routableUserChatPrefix,
                                           ref(m_face),
                                           bind(&ChatDialogBackend::processSyncUpdate, this, _1));

  // schedule a new join event
  m_scheduler.scheduleEvent(time::milliseconds(600),
                            bind(&ChatDialogBackend::sendJoin, this));

  // cancel existing hello event if it exists
  if (static_cast<bool>(m_helloEventId)) {
    m_scheduler.cancelEvent(m_helloEventId);
    m_helloEventId.reset();
  }
}

void
ChatDialogBackend::processSyncUpdate(const std::vector<chronosync::MissingDataInfo>& updates)
{
  _LOG_DEBUG("<<< processing Tree Update");

  if (updates.empty()) {
    return;
  }

  std::vector<NodeInfo> nodeInfos;


  for (int i = 0; i < updates.size(); i++) {
    // update roster
    if (m_roster.find(updates[i].session) == m_roster.end()) {
      m_roster[updates[i].session].sessionPrefix = updates[i].session;
      m_roster[updates[i].session].hasNick = false;
    }

    // fetch missing chat data
    if (updates[i].high - updates[i].low < 3) {
      for (chronosync::SeqNo seq = updates[i].low; seq <= updates[i].high; ++seq) {
        m_sock->fetchData(updates[i].session, seq,
                          bind(&ChatDialogBackend::processChatData, this, _1, true),
                          2);
        _LOG_DEBUG("<<< Fetching " << updates[i].session << "/" << seq);
      }
    }
    else {
      // There are too many msgs to fetch, let's just fetch the latest one
      m_sock->fetchData(updates[i].session, updates[i].high,
                        bind(&ChatDialogBackend::processChatData, this, _1, false),
                        2);
    }

    // prepare notification to frontend
    NodeInfo nodeInfo;
    nodeInfo.sessionPrefix = QString::fromStdString(updates[i].session.toUri());
    nodeInfo.seqNo = updates[i].high;
    nodeInfos.push_back(nodeInfo);
  }

  // reflect the changes on GUI
  emit syncTreeUpdated(nodeInfos,
                       QString::fromStdString(getHexEncodedDigest(m_sock->getRootDigest())));
}

void
ChatDialogBackend::processChatData(const ndn::shared_ptr<const ndn::Data>& data, bool needDisplay)
{
  SyncDemo::ChatMessage msg;

  if (!msg.ParseFromArray(data->getContent().value(), data->getContent().value_size())) {
    _LOG_DEBUG("Errrrr.. Can not parse msg with name: " <<
               data->getName() << ". what is happening?");
    // nasty stuff: as a remedy, we'll form some standard msg for inparsable msgs
    msg.set_from("inconnu");
    msg.set_type(SyncDemo::ChatMessage::OTHER);
    return;
  }

  Name remoteSessionPrefix = data->getName().getPrefix(-1);

  if (msg.type() == SyncDemo::ChatMessage::LEAVE) {
    BackendRoster::iterator it = m_roster.find(remoteSessionPrefix);

    if (it != m_roster.end()) {
      // cancel timeout event
      if (static_cast<bool>(it->second.timeoutEventId))
        m_scheduler.cancelEvent(it->second.timeoutEventId);

      // notify frontend to remove the remote session (node)
      emit sessionRemoved(QString::fromStdString(remoteSessionPrefix.toUri()),
                          QString::fromStdString(msg.from()),
                          msg.timestamp());

      // remove roster entry
      m_roster.erase(remoteSessionPrefix);
    }
  }
  else {
    BackendRoster::iterator it = m_roster.find(remoteSessionPrefix);

    if (it == m_roster.end()) {
      // Should not happen
      BOOST_ASSERT(false);
    }

    // If we haven't got any message from this session yet.
    if (m_roster[remoteSessionPrefix].hasNick == false) {
      m_roster[remoteSessionPrefix].userNick = msg.from();
      m_roster[remoteSessionPrefix].hasNick = true;
      emit sessionAdded(QString::fromStdString(remoteSessionPrefix.toUri()),
                        QString::fromStdString(msg.from()),
                        msg.timestamp());
    }

    // If we get a new nick for an existing session, update it.
    if (m_roster[remoteSessionPrefix].userNick != msg.from()) {
      m_roster[remoteSessionPrefix].userNick = msg.from();
      emit nickUpdated(QString::fromStdString(remoteSessionPrefix.toUri()),
                       QString::fromStdString(msg.from()));
    }

    // If a timeout event has been scheduled, cancel it.
    if (static_cast<bool>(it->second.timeoutEventId))
      m_scheduler.cancelEvent(it->second.timeoutEventId);

    // (Re)schedule another timeout event after 3 HELLO_INTERVAL;
    it->second.timeoutEventId =
      m_scheduler.scheduleEvent(HELLO_INTERVAL * 3,
                                bind(&ChatDialogBackend::remoteSessionTimeout,
                                     this, remoteSessionPrefix));

    // If chat message, notify the frontend
    if (msg.type() == SyncDemo::ChatMessage::CHAT)
      emit chatMessageReceived(QString::fromStdString(msg.from()),
                               QString::fromStdString(msg.data()),
                               msg.timestamp());

    // Notify frontend to plot notification on DigestTree.
    emit messageReceived(QString::fromStdString(remoteSessionPrefix.toUri()));
  }
}

void
ChatDialogBackend::remoteSessionTimeout(const Name& sessionPrefix)
{
  time_t timestamp =
    static_cast<time_t>(time::toUnixTimestamp(time::system_clock::now()).count() / 1000);

  // notify frontend
  emit sessionRemoved(QString::fromStdString(sessionPrefix.toUri()),
                      QString::fromStdString(m_roster[sessionPrefix].userNick),
                      timestamp);

  // remove roster entry
  m_roster.erase(sessionPrefix);
}

void
ChatDialogBackend::sendMsg(SyncDemo::ChatMessage& msg)
{
  // send msg
  ndn::OBufferStream os;
  msg.SerializeToOstream(&os);

  if (!msg.IsInitialized()) {
    _LOG_DEBUG("Errrrr.. msg was not probally initialized " << __FILE__ <<
               ":" << __LINE__ << ". what is happening?");
    abort();
  }

  uint64_t nextSequence = m_sock->getLogic().getSeqNo() + 1;

  m_sock->publishData(os.buf()->buf(), os.buf()->size(), FRESHNESS_PERIOD);

  std::vector<NodeInfo> nodeInfos;
  NodeInfo nodeInfo = {QString::fromStdString(m_routableUserChatPrefix.toUri()),
                       nextSequence};
  nodeInfos.push_back(nodeInfo);

  emit syncTreeUpdated(nodeInfos,
                       QString::fromStdString(getHexEncodedDigest(m_sock->getRootDigest())));
}

void
ChatDialogBackend::sendJoin()
{
  m_joined = true;

  SyncDemo::ChatMessage msg;
  prepareControlMessage(msg, SyncDemo::ChatMessage::JOIN);
  sendMsg(msg);

  m_helloEventId = m_scheduler.scheduleEvent(HELLO_INTERVAL,
                                             bind(&ChatDialogBackend::sendHello, this));

  emit sessionAdded(QString::fromStdString(m_routableUserChatPrefix.toUri()),
                    QString::fromStdString(msg.from()),
                    msg.timestamp());
}

void
ChatDialogBackend::sendHello()
{
  SyncDemo::ChatMessage msg;
  prepareControlMessage(msg, SyncDemo::ChatMessage::HELLO);
  sendMsg(msg);

  m_helloEventId = m_scheduler.scheduleEvent(HELLO_INTERVAL,
                                             bind(&ChatDialogBackend::sendHello, this));
}

void
ChatDialogBackend::sendLeave()
{
  SyncDemo::ChatMessage msg;
  prepareControlMessage(msg, SyncDemo::ChatMessage::LEAVE);
  sendMsg(msg);

  usleep(5000);
  m_joined = false;
}

void
ChatDialogBackend::prepareControlMessage(SyncDemo::ChatMessage& msg,
                                         SyncDemo::ChatMessage::ChatMessageType type)
{
  msg.set_from(m_nick);
  msg.set_to(m_chatroomName);
  int32_t seconds =
    static_cast<int32_t>(time::toUnixTimestamp(time::system_clock::now()).count() / 1000);
  msg.set_timestamp(seconds);
  msg.set_type(type);
}

void
ChatDialogBackend::prepareChatMessage(const QString& text,
                                      time_t timestamp,
                                      SyncDemo::ChatMessage &msg)
{
  msg.set_from(m_nick);
  msg.set_to(m_chatroomName);
  msg.set_data(text.toStdString());
  msg.set_timestamp(timestamp);
  msg.set_type(SyncDemo::ChatMessage::CHAT);
}

void
ChatDialogBackend::updatePrefixes()
{
  m_routableUserChatPrefix.clear();

  if (m_localRoutingPrefix.isPrefixOf(m_userChatPrefix))
    m_routableUserChatPrefix = m_userChatPrefix;
  else
    m_routableUserChatPrefix.append(m_localRoutingPrefix)
      .append(ROUTING_HINT_SEPARATOR, 2)
      .append(m_userChatPrefix);

  emit chatPrefixChanged(m_routableUserChatPrefix);
}

std::string
ChatDialogBackend::getHexEncodedDigest(ndn::ConstBufferPtr digest)
{
  std::stringstream os;

  CryptoPP::StringSource(digest->buf(), digest->size(), true,
                         new CryptoPP::HexEncoder(new CryptoPP::FileSink(os), false));
  return os.str();
}


// public slots:
void
ChatDialogBackend::sendChatMessage(QString text, time_t timestamp)
{
  SyncDemo::ChatMessage msg;
  prepareChatMessage(text, timestamp, msg);
  sendMsg(msg);

  emit chatMessageReceived(QString::fromStdString(msg.from()),
                           QString::fromStdString(msg.data()),
                           msg.timestamp());
}

void
ChatDialogBackend::updateRoutingPrefix(const QString& localRoutingPrefix)
{
  Name newLocalRoutingPrefix(localRoutingPrefix.toStdString());

  if (!newLocalRoutingPrefix.empty() && newLocalRoutingPrefix != m_localRoutingPrefix) {
    // Update localPrefix
    m_localRoutingPrefix = newLocalRoutingPrefix;

    updatePrefixes();

    initializeSync();
  }
}

void
ChatDialogBackend::shutdown()
{
  if (static_cast<bool>(m_sock)) {
    if (m_joined)
      sendLeave();
    m_sock.reset();

    usleep(100000);
  }

  m_face.getIoService().stop();
}

} // namespace chronos

#if WAF
#include "chat-dialog-backend.moc"
// #include "chat-dialog-backend.cpp.moc"
#endif
