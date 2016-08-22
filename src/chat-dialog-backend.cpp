/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013-2016, Regents of the University of California
 *                          Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 *         Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#include "chat-dialog-backend.hpp"

#include <QFile>

#ifndef Q_MOC_RUN
#include <boost/iostreams/stream.hpp>
#include <ndn-cxx/util/io.hpp>
#include <ndn-cxx/security/validator-regex.hpp>
#include "logging.h"
#endif


INIT_LOGGER("ChatDialogBackend");

namespace chronochat {

static const time::milliseconds FRESHNESS_PERIOD(60000);
static const time::seconds HELLO_INTERVAL(60);
static const ndn::Name::Component ROUTING_HINT_SEPARATOR =
  ndn::name::Component::fromEscapedString("%F0%2E");
static const int IDENTITY_OFFSET = -3;
static const int CONNECTION_RETRY_TIMER = 3;

ChatDialogBackend::ChatDialogBackend(const Name& chatroomPrefix,
                                     const Name& userChatPrefix,
                                     const Name& routingPrefix,
                                     const std::string& chatroomName,
                                     const std::string& nick,
                                     const Name& signingId,
                                     QObject* parent)
  : QThread(parent)
  , m_shouldResume(false)
  , m_localRoutingPrefix(routingPrefix)
  , m_chatroomPrefix(chatroomPrefix)
  , m_userChatPrefix(userChatPrefix)
  , m_chatroomName(chatroomName)
  , m_nick(nick)
  , m_signingId(signingId)
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
  bool shouldResume = false;
  do {
    initializeSync();

    if (m_face == nullptr)
      break;

    try {
      m_face->getIoService().run();
    }
    catch (std::runtime_error& e) {
      {
        std::lock_guard<std::mutex>lock(m_nfdConnectionMutex);
        m_isNfdConnected = false;
      }
      emit nfdError();
      {
        std::lock_guard<std::mutex>lock(m_resumeMutex);
        m_shouldResume = true;
      }
#ifdef BOOST_THREAD_USES_CHRONO
      time::seconds reconnectTimer = time::seconds(CONNECTION_RETRY_TIMER);
#else
      boost::posix_time::time_duration reconnectTimer;
      reconnectTimer = boost::posix_time::seconds(CONNECTION_RETRY_TIMER);
#endif
      while (!m_isNfdConnected) {
#ifdef BOOST_THREAD_USES_CHRONO
        boost::this_thread::sleep_for(reconnectTimer);
#else
        boost::this_thread::sleep(reconnectTimer);
#endif
      }
      emit refreshChatDialog(m_routableUserChatPrefix);
    }
    {
      std::lock_guard<std::mutex>lock(m_resumeMutex);
      shouldResume = m_shouldResume;
      m_shouldResume = false;
    }
    close();

  } while (shouldResume);

  std::cerr << "Bye!" << std::endl;
}

// private methods:
void
ChatDialogBackend::initializeSync()
{
  BOOST_ASSERT(m_sock == nullptr);

  m_face = make_shared<ndn::Face>();
  m_scheduler = unique_ptr<ndn::Scheduler>(new ndn::Scheduler(m_face->getIoService()));

  // initialize validator
  shared_ptr<ndn::IdentityCertificate> anchor = loadTrustAnchor();

  if (static_cast<bool>(anchor)) {
    shared_ptr<ndn::ValidatorRegex> validator =
      make_shared<ndn::ValidatorRegex>(m_face.get()); // TODO: Change to Face*
    validator->addDataVerificationRule(
      make_shared<ndn::SecRuleRelative>("^<>*<%F0.>(<>*)$",
                                        "^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>$",
                                        ">", "\\1", "\\1\\2", true));
    validator->addDataVerificationRule(
      make_shared<ndn::SecRuleRelative>("(<>*)$",
                                        "^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>$",
                                        ">", "\\1", "\\1\\2", true));
    validator->addTrustAnchor(anchor);

    m_validator = validator;
  }
  else
    m_validator = shared_ptr<ndn::Validator>();


  // create a new SyncSocket
  m_sock = make_shared<chronosync::Socket>(m_chatroomPrefix,
                                           m_routableUserChatPrefix,
                                           ref(*m_face),
                                           bind(&ChatDialogBackend::processSyncUpdate, this, _1),
                                           m_signingId,
                                           m_validator);

  // schedule a new join event
  m_scheduler->scheduleEvent(time::milliseconds(600),
                             bind(&ChatDialogBackend::sendJoin, this));

  // cancel existing hello event if it exists
  if (m_helloEventId != nullptr) {
    m_scheduler->cancelEvent(m_helloEventId);
    m_helloEventId.reset();
  }
}

class IoDeviceSource
{
public:
  typedef char char_type;
  typedef boost::iostreams::source_tag category;

  explicit
  IoDeviceSource(QIODevice& source)
    : m_source(source)
  {
  }

  std::streamsize
  read(char* buffer, std::streamsize n)
  {
    return m_source.read(buffer, n);
  }
private:
  QIODevice& m_source;
};

shared_ptr<ndn::IdentityCertificate>
ChatDialogBackend::loadTrustAnchor()
{
  QFile anchorFile(":/security/anchor.cert");

  if (!anchorFile.open(QIODevice::ReadOnly)) {
    return {};
  }

  boost::iostreams::stream<IoDeviceSource> anchorFileStream(anchorFile);
  return ndn::io::load<ndn::IdentityCertificate>(anchorFileStream);
}

void
ChatDialogBackend::exitChatroom() {
  if (m_joined)
    sendLeave();

  usleep(100000);
}

void
ChatDialogBackend::close()
{
  m_scheduler->cancelAllEvents();
  m_helloEventId.reset();
  m_roster.clear();
  m_validator.reset();
  m_sock.reset();
}

void
ChatDialogBackend::processSyncUpdate(const std::vector<chronosync::MissingDataInfo>& updates)
{
  _LOG_DEBUG("<<< processing Tree Update");

  if (updates.empty()) {
    return;
  }

  std::vector<NodeInfo> nodeInfos;


  for (size_t i = 0; i < updates.size(); i++) {
    // update roster
    if (m_roster.find(updates[i].session) == m_roster.end()) {
      m_roster[updates[i].session].sessionPrefix = updates[i].session;
      m_roster[updates[i].session].hasNick = false;
    }

    // fetch missing chat data
    if (updates[i].high - updates[i].low < 3) {
      for (chronosync::SeqNo seq = updates[i].low; seq <= updates[i].high; ++seq) {
        m_sock->fetchData(updates[i].session, seq,
                          [this] (const shared_ptr<const ndn::Data>& data) {
                            this->processChatData(data, true, true);
                          },
                          [this] (const shared_ptr<const ndn::Data>& data, const std::string& msg) {
                            this->processChatData(data, true, false);
                          },
                          ndn::OnTimeout(),
                          2);
        _LOG_DEBUG("<<< Fetching " << updates[i].session << "/" << seq);
      }
    }
    else {
      // There are too many msgs to fetch, let's just fetch the latest one
      m_sock->fetchData(updates[i].session, updates[i].high,
                        [this] (const shared_ptr<const ndn::Data>& data) {
                          this->processChatData(data, false, true);
                        },
                        [this] (const shared_ptr<const ndn::Data>& data, const std::string& msg) {
                          this->processChatData(data, false, false);
                        },
                        ndn::OnTimeout(),
                        2);
    }

  }

  // reflect the changes on GUI
  emit syncTreeUpdated(nodeInfos,
                       QString::fromStdString(getHexEncodedDigest(m_sock->getRootDigest())));
}

void
ChatDialogBackend::processChatData(const ndn::shared_ptr<const ndn::Data>& data,
                                   bool needDisplay,
                                   bool isValidated)
{
  ChatMessage msg;

  try {
    msg.wireDecode(data->getContent().blockFromValue());
  }
  catch (tlv::Error) {
    _LOG_DEBUG("Errrrr.. Can not parse msg with name: " <<
               data->getName() << ". what is happening?");
    // nasty stuff: as a remedy, we'll form some standard msg for inparsable msgs
    msg.setNick("inconnu");
    msg.setMsgType(ChatMessage::OTHER);
    return;
  }

  Name remoteSessionPrefix = data->getName().getPrefix(-1);

  if (msg.getMsgType() == ChatMessage::LEAVE) {
    BackendRoster::iterator it = m_roster.find(remoteSessionPrefix);

    if (it != m_roster.end()) {
      // cancel timeout event
      if (static_cast<bool>(it->second.timeoutEventId))
        m_scheduler->cancelEvent(it->second.timeoutEventId);

      // notify frontend to remove the remote session (node)
      emit sessionRemoved(QString::fromStdString(remoteSessionPrefix.toUri()),
                          QString::fromStdString(msg.getNick()),
                          msg.getTimestamp());

      // remove roster entry
      m_roster.erase(remoteSessionPrefix);

      emit eraseInRoster(remoteSessionPrefix.getPrefix(IDENTITY_OFFSET),
                         Name::Component(m_chatroomName));
    }
  }
  else {
    BackendRoster::iterator it = m_roster.find(remoteSessionPrefix);

    if (it == m_roster.end()) {
      // Should not happen
      BOOST_ASSERT(false);
    }

    uint64_t seqNo = data->getName().get(-1).toNumber();

    // If a timeout event has been scheduled, cancel it.
    if (static_cast<bool>(it->second.timeoutEventId))
      m_scheduler->cancelEvent(it->second.timeoutEventId);

    // (Re)schedule another timeout event after 3 HELLO_INTERVAL;
    it->second.timeoutEventId =
      m_scheduler->scheduleEvent(HELLO_INTERVAL * 3,
                                 bind(&ChatDialogBackend::remoteSessionTimeout,
                                      this, remoteSessionPrefix));

    // If chat message, notify the frontend
    if (msg.getMsgType() == ChatMessage::CHAT) {
      if (isValidated)
        emit chatMessageReceived(QString::fromStdString(msg.getNick()),
                                 QString::fromStdString(msg.getData()),
                                 msg.getTimestamp());
      else
        emit chatMessageReceived(QString::fromStdString(msg.getNick() + " (Unverified)"),
                                 QString::fromStdString(msg.getData()),
                                 msg.getTimestamp());
    }

    // Notify frontend to plot notification on DigestTree.

    // If we haven't got any message from this session yet.
    if (m_roster[remoteSessionPrefix].hasNick == false) {
      m_roster[remoteSessionPrefix].userNick = msg.getNick();
      m_roster[remoteSessionPrefix].hasNick = true;

      emit messageReceived(QString::fromStdString(remoteSessionPrefix.toUri()),
                           QString::fromStdString(msg.getNick()),
                           seqNo,
                           msg.getTimestamp(),
                           true);

      emit addInRoster(remoteSessionPrefix.getPrefix(IDENTITY_OFFSET),
                       Name::Component(m_chatroomName));
    }
    else
      emit messageReceived(QString::fromStdString(remoteSessionPrefix.toUri()),
                           QString::fromStdString(msg.getNick()),
                           seqNo,
                           msg.getTimestamp(),
                           false);
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

  emit eraseInRoster(sessionPrefix.getPrefix(IDENTITY_OFFSET),
                     Name::Component(m_chatroomName));
}

void
ChatDialogBackend::sendMsg(ChatMessage& msg)
{
  // send msg
  ndn::Block buf = msg.wireEncode();

  uint64_t nextSequence = m_sock->getLogic().getSeqNo() + 1;

  m_sock->publishData(buf.wire(), buf.size(), FRESHNESS_PERIOD);

  std::vector<NodeInfo> nodeInfos;
  Name sessionName = m_sock->getLogic().getSessionName();
  NodeInfo nodeInfo = {QString::fromStdString(sessionName.toUri()),
                       nextSequence};
  nodeInfos.push_back(nodeInfo);

  emit syncTreeUpdated(nodeInfos,
                       QString::fromStdString(getHexEncodedDigest(m_sock->getRootDigest())));

  emit messageReceived(QString::fromStdString(sessionName.toUri()),
                       QString::fromStdString(msg.getNick()),
                       nextSequence,
                       msg.getTimestamp(),
                       msg.getMsgType() == ChatMessage::JOIN);
}

void
ChatDialogBackend::sendJoin()
{
  m_joined = true;

  ChatMessage msg;
  prepareControlMessage(msg, ChatMessage::JOIN);
  sendMsg(msg);

  m_helloEventId = m_scheduler->scheduleEvent(HELLO_INTERVAL,
                                              bind(&ChatDialogBackend::sendHello, this));
  emit newChatroomForDiscovery(Name::Component(m_chatroomName));
}

void
ChatDialogBackend::sendHello()
{
  ChatMessage msg;
  prepareControlMessage(msg, ChatMessage::HELLO);
  sendMsg(msg);

  m_helloEventId = m_scheduler->scheduleEvent(HELLO_INTERVAL,
                                              bind(&ChatDialogBackend::sendHello, this));
}

void
ChatDialogBackend::sendLeave()
{
  ChatMessage msg;
  prepareControlMessage(msg, ChatMessage::LEAVE);
  sendMsg(msg);

  // get my own identity with routable prefix by getPrefix(-2)
  emit eraseInRoster(m_routableUserChatPrefix.getPrefix(-2),
                     Name::Component(m_chatroomName));

  usleep(5000);
  m_joined = false;
}

void
ChatDialogBackend::prepareControlMessage(ChatMessage& msg,
                                         ChatMessage::ChatMessageType type)
{
  msg.setNick(m_nick);
  msg.setChatroomName(m_chatroomName);
  int32_t seconds =
    static_cast<int32_t>(time::toUnixTimestamp(time::system_clock::now()).count() / 1000);
  msg.setTimestamp(seconds);
  msg.setMsgType(type);
}

void
ChatDialogBackend::prepareChatMessage(const QString& text,
                                      time_t timestamp,
                                      ChatMessage &msg)
{
  msg.setNick(m_nick);
  msg.setChatroomName(m_chatroomName);
  msg.setData(text.toStdString());
  msg.setTimestamp(timestamp);
  msg.setMsgType(ChatMessage::CHAT);
}

void
ChatDialogBackend::updatePrefixes()
{
  m_routableUserChatPrefix.clear();

  if (m_localRoutingPrefix.isPrefixOf(m_userChatPrefix))
    m_routableUserChatPrefix = m_userChatPrefix;
  else
    m_routableUserChatPrefix.append(m_localRoutingPrefix)
      .append(ROUTING_HINT_SEPARATOR)
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
  ChatMessage msg;
  prepareChatMessage(text, timestamp, msg);
  sendMsg(msg);

  emit chatMessageReceived(QString::fromStdString(msg.getNick()),
                           QString::fromStdString(msg.getData()),
                           msg.getTimestamp());
}

void
ChatDialogBackend::updateRoutingPrefix(const QString& localRoutingPrefix)
{
  Name newLocalRoutingPrefix(localRoutingPrefix.toStdString());

  if (!newLocalRoutingPrefix.empty() && newLocalRoutingPrefix != m_localRoutingPrefix) {
    // Update localPrefix
    m_localRoutingPrefix = newLocalRoutingPrefix;

    {
      std::lock_guard<std::mutex>lock(m_resumeMutex);
      m_shouldResume = true;
    }

    exitChatroom();

    updatePrefixes();

    m_face->getIoService().stop();
  }
}

void
ChatDialogBackend::shutdown()
{
  {
    std::lock_guard<std::mutex>lock(m_resumeMutex);
    m_shouldResume = false;
  }

  {
    // In this case, we just stop checking the nfd connection and exit
    std::lock_guard<std::mutex>lock(m_nfdConnectionMutex);
    m_isNfdConnected = true;
  }

  exitChatroom();

  m_face->getIoService().stop();
}

void
ChatDialogBackend::onNfdReconnect()
{
  std::lock_guard<std::mutex>lock(m_nfdConnectionMutex);
  m_isNfdConnected = true;
}

} // namespace chronochat

#if WAF
#include "chat-dialog-backend.moc"
// #include "chat-dialog-backend.cpp.moc"
#endif
