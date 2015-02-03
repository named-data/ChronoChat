/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 *         Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "chatroom-discovery-backend.hpp"
#include <QStringList>


#ifndef Q_MOC_RUN

#endif

namespace chronochat {

static const time::milliseconds FRESHNESS_PERIOD(60000);
static const time::seconds REFRESH_INTERVAL(60);
static const time::seconds HELLO_INTERVAL(60);
static const ndn::Name::Component ROUTING_HINT_SEPARATOR =
  ndn::name::Component::fromEscapedString("%F0%2E");
// a count enforced when a manager himself find another one publish chatroom data
static const int MAXIMUM_COUNT = 3;
static const int IDENTITY_OFFSET = -1;
static const int CONNECTION_RETRY_TIMER = 3;

ChatroomDiscoveryBackend::ChatroomDiscoveryBackend(const Name& routingPrefix,
                                                   const Name& identity,
                                                   QObject* parent)
  : QThread(parent)
  , m_shouldResume(false)
  , m_routingPrefix(routingPrefix)
  , m_identity(identity)
  , m_randomGenerator(static_cast<unsigned int>(std::time(0)))
  , m_rangeUniformRandom(m_randomGenerator, boost::uniform_int<>(500,2000))
{
  m_discoveryPrefix.append("ndn")
    .append("broadcast")
    .append("ChronoChat")
    .append("Discovery");
  m_userDiscoveryPrefix.append(m_identity).append("CHRONOCHAT-DISCOVERYDATA");
  updatePrefixes();
}

ChatroomDiscoveryBackend::~ChatroomDiscoveryBackend()
{
}

void
ChatroomDiscoveryBackend::run()
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
      boost::posix_time::time_duration reconnectTimer = boost::posix_time::seconds(CONNECTION_RETRY_TIMER);
#endif
      while (!m_isNfdConnected) {
#ifdef BOOST_THREAD_USES_CHRONO
        boost::this_thread::sleep_for(reconnectTimer);
#else
        boost::this_thread::sleep(reconnectTimer);
#endif
      }
    }
    {
      std::lock_guard<std::mutex>lock(m_resumeMutex);
      shouldResume = m_shouldResume;
      m_shouldResume = false;
    }
    close();

  } while (shouldResume);

  std::cerr << "DiscoveryBackend: Bye!" << std::endl;
}

void
ChatroomDiscoveryBackend::initializeSync()
{
  BOOST_ASSERT(m_sock == nullptr);

  m_face = shared_ptr<ndn::Face>(new ndn::Face);
  m_scheduler = unique_ptr<ndn::Scheduler>(new ndn::Scheduler(m_face->getIoService()));

  m_sock = make_shared<chronosync::Socket>(m_discoveryPrefix,
                                           Name(),
                                           ref(*m_face),
                                           bind(&ChatroomDiscoveryBackend::processSyncUpdate,
                                                this, _1));

  // add an timer to refresh front end
  if (m_refreshPanelId != nullptr) {
    m_scheduler->cancelEvent(m_refreshPanelId);
  }
  m_refreshPanelId = m_scheduler->scheduleEvent(REFRESH_INTERVAL,
                                                [this] { sendChatroomList(); });
}

void
ChatroomDiscoveryBackend::close()
{
  m_scheduler->cancelAllEvents();
  m_refreshPanelId.reset();
  m_chatroomList.clear();
  m_sock.reset();
}

void
ChatroomDiscoveryBackend::processSyncUpdate(const std::vector<chronosync::MissingDataInfo>& updates)
{
  if (updates.empty()) {
    return;
  }
  for (const auto& update : updates) {
    m_sock->fetchData(update.session, update.high,
                      bind(&ChatroomDiscoveryBackend::processChatroomData, this, _1), 2);
  }
}

void
ChatroomDiscoveryBackend::processChatroomData(const ndn::shared_ptr<const ndn::Data>& data)
{
  // extract chatroom name by get(-3)
  Name::Component chatroomName = data->getName().get(-3);
  auto it = m_chatroomList.find(chatroomName);
  if (it == m_chatroomList.end()) {
    m_chatroomList[chatroomName].chatroomName = chatroomName.toUri();
    m_chatroomList[chatroomName].count = 0;
    m_chatroomList[chatroomName].isPrint = false;
    m_chatroomList[chatroomName].isParticipant = false;
    m_chatroomList[chatroomName].isManager = false;
    it = m_chatroomList.find(chatroomName);
  }
  // If the user is the manager of this chatroom, he should not receive any data from this chatroom
  if (it->second.isManager) {
    if (it->second.count < MAXIMUM_COUNT) {
      it->second.count++;
      return;
    }
    else {
      it->second.count = 0;
      if (m_routableUserDiscoveryPrefix < data->getName()) {
        // when two managers exist, the one with "smaller" name take the control
        sendUpdate(chatroomName);
        return;
      }
      else {
        if (it->second.helloTimeoutEventId != nullptr) {
          m_scheduler->cancelEvent(it->second.helloTimeoutEventId);
        }
        it->second.helloTimeoutEventId = nullptr;
        it->second.isManager = false;
      }

    }
  }

  else if (it->second.isParticipant) {
    if (it->second.localChatroomTimeoutEventId != nullptr)
      m_scheduler->cancelEvent(it->second.localChatroomTimeoutEventId);

    // If a user start a random timer it means that he think his own chatroom is not alive
    // But when he receive some packet, it means that this chatroom is alive, so he can
    // cancel the timer
    if (it->second.managerSelectionTimeoutEventId != nullptr)
      m_scheduler->cancelEvent(it->second.managerSelectionTimeoutEventId);
    it->second.managerSelectionTimeoutEventId = nullptr;

    it->second.localChatroomTimeoutEventId =
      m_scheduler->scheduleEvent(HELLO_INTERVAL * 3,
                                 bind(&ChatroomDiscoveryBackend::localSessionTimeout,
                                      this, chatroomName));
  }
  else {
    if (!data->getContent().empty()) {
      ChatroomInfo chatroom;
      chatroom.wireDecode(data->getContent().blockFromValue());
      it->second.info = chatroom;
    }

    if (it->second.remoteChatroomTimeoutEventId != nullptr)
      m_scheduler->cancelEvent(it->second.remoteChatroomTimeoutEventId);

    it->second.remoteChatroomTimeoutEventId =
      m_scheduler->scheduleEvent(HELLO_INTERVAL * 5,
                                 bind(&ChatroomDiscoveryBackend::remoteSessionTimeout,
                                      this, chatroomName));
  }
  // if this is a chatroom that haven't been print on the discovery panel, print it.
  if(!it->second.isPrint) {
    sendChatroomList();
    it->second.isPrint = true;
  }
}

void
ChatroomDiscoveryBackend::localSessionTimeout(const Name::Component& chatroomName)
{
  auto it = m_chatroomList.find(chatroomName);
  if (it == m_chatroomList.end() || it->second.isParticipant == false)
    return;
  it->second.managerSelectionTimeoutEventId =
    m_scheduler->scheduleEvent(time::milliseconds(m_rangeUniformRandom()),
                               bind(&ChatroomDiscoveryBackend::randomSessionTimeout,
                                    this, chatroomName));
}

void
ChatroomDiscoveryBackend::remoteSessionTimeout(const Name::Component& chatroomName)
{
  m_chatroomList.erase(chatroomName);
}

void
ChatroomDiscoveryBackend::randomSessionTimeout(const Name::Component& chatroomName)
{
  Name prefix = m_routableUserDiscoveryPrefix;
  prefix.append(chatroomName);
  m_sock->addSyncNode(prefix);

  emit chatroomInfoRequest(chatroomName.toUri(), true);
}

void
ChatroomDiscoveryBackend::sendUpdate(const Name::Component& chatroomName)
{
  auto it = m_chatroomList.find(chatroomName);
  if (it != m_chatroomList.end() && it->second.isManager) {
    ndn::Block buf = it->second.info.wireEncode();

    if (it->second.helloTimeoutEventId != nullptr) {
      m_scheduler->cancelEvent(it->second.helloTimeoutEventId);
    }

    m_sock->publishData(buf.wire(), buf.size(), FRESHNESS_PERIOD, it->second.chatroomPrefix);

    it->second.helloTimeoutEventId =
      m_scheduler->scheduleEvent(HELLO_INTERVAL,
                                 bind(&ChatroomDiscoveryBackend::sendUpdate, this, chatroomName));
    // if this is a chatroom that haven't been print on the discovery panel, print it.
    if(!it->second.isPrint) {
      sendChatroomList();
      it->second.isPrint = true;
    }
  }
}

void
ChatroomDiscoveryBackend::updatePrefixes()
{
  Name temp;
  if (m_routingPrefix.isPrefixOf(m_userDiscoveryPrefix))
    temp = m_userDiscoveryPrefix;
  else
    temp.append(m_routingPrefix)
      .append(ROUTING_HINT_SEPARATOR)
      .append(m_userDiscoveryPrefix);

  Name routableIdentity = m_routableUserDiscoveryPrefix.getPrefix(IDENTITY_OFFSET);
  for (auto& chatroom : m_chatroomList) {
    if (chatroom.second.isParticipant) {
      chatroom.second.info.removeParticipant(routableIdentity);
      chatroom.second.info.addParticipant(temp.getPrefix(IDENTITY_OFFSET));
    }
  }
  m_routableUserDiscoveryPrefix = temp;
}

void
ChatroomDiscoveryBackend::updateRoutingPrefix(const QString& routingPrefix)
{
  Name newRoutingPrefix(routingPrefix.toStdString());
  if (!newRoutingPrefix.empty() && newRoutingPrefix != m_routingPrefix) {
    // Update localPrefix
    m_routingPrefix = newRoutingPrefix;

    updatePrefixes();

    {
      std::lock_guard<std::mutex>lock(m_resumeMutex);
      m_shouldResume = true;
    }

    m_face->getIoService().stop();
  }
}

void
ChatroomDiscoveryBackend::onEraseInRoster(ndn::Name sessionPrefix,
                                          ndn::Name::Component chatroomName)
{
  auto it = m_chatroomList.find(chatroomName);
  if (it != m_chatroomList.end()) {
    it->second.info.removeParticipant(sessionPrefix);
    if (it->second.info.getParticipants().size() == 0) {
      // Before deleting the chatroom, cancel the hello event timer if exist
      if (it->second.helloTimeoutEventId != nullptr)
        m_scheduler->cancelEvent(it->second.helloTimeoutEventId);

      m_chatroomList.erase(chatroomName);
      Name prefix = sessionPrefix;
      prefix.append("CHRONOCHAT-DISCOVERYDATA").append(chatroomName);
      m_sock->removeSyncNode(prefix);
      sendChatroomList();
      return;
    }

    if (sessionPrefix.isPrefixOf(m_routableUserDiscoveryPrefix)) {
      it->second.isParticipant = false;
      it->second.isManager = false;
      it->second.isPrint = false;
      it->second.count = 0;
      if (it->second.helloTimeoutEventId != nullptr)
        m_scheduler->cancelEvent(it->second.helloTimeoutEventId);
      it->second.helloTimeoutEventId = nullptr;

      if (it->second.localChatroomTimeoutEventId != nullptr)
        m_scheduler->cancelEvent(it->second.localChatroomTimeoutEventId);
      it->second.localChatroomTimeoutEventId = nullptr;

      it->second.remoteChatroomTimeoutEventId =
      m_scheduler->scheduleEvent(HELLO_INTERVAL * 5,
                                bind(&ChatroomDiscoveryBackend::remoteSessionTimeout,
                                     this, chatroomName));
    }

    if (it->second.isManager) {
      sendUpdate(chatroomName);
    }
  }
}

void
ChatroomDiscoveryBackend::onAddInRoster(ndn::Name sessionPrefix,
                                        ndn::Name::Component chatroomName)
{
  auto it = m_chatroomList.find(chatroomName);
  if (it != m_chatroomList.end()) {
    it->second.info.addParticipant(sessionPrefix);
    if (it->second.isManager)
      sendUpdate(chatroomName);
  }
  else {
    onNewChatroomForDiscovery(chatroomName);
  }
}

void
ChatroomDiscoveryBackend::onNewChatroomForDiscovery(ndn::Name::Component chatroomName)
{
  Name newPrefix = m_routableUserDiscoveryPrefix;
  newPrefix.append(chatroomName);
  auto it = m_chatroomList.find(chatroomName);
  if (it == m_chatroomList.end()) {
    m_chatroomList[chatroomName].chatroomPrefix = newPrefix;
    m_chatroomList[chatroomName].isParticipant = true;
    m_chatroomList[chatroomName].isManager = false;
    m_chatroomList[chatroomName].count = 0;
    m_chatroomList[chatroomName].isPrint = false;
    m_scheduler->scheduleEvent(time::milliseconds(600),
                               bind(&ChatroomDiscoveryBackend::randomSessionTimeout, this,
                                    chatroomName));
  }
  else {
    // Entering an existing chatroom
    it->second.isParticipant = true;
    it->second.isManager = false;
    it->second.chatroomPrefix = newPrefix;

    if (it->second.remoteChatroomTimeoutEventId != nullptr)
      m_scheduler->cancelEvent(it->second.remoteChatroomTimeoutEventId);
    it->second.isPrint = false;
    it->second.remoteChatroomTimeoutEventId = nullptr;

    it->second.localChatroomTimeoutEventId =
      m_scheduler->scheduleEvent(HELLO_INTERVAL * 3,
                                 bind(&ChatroomDiscoveryBackend::localSessionTimeout,
                                      this, chatroomName));
    emit chatroomInfoRequest(chatroomName.toUri(), false);
  }
}

void
ChatroomDiscoveryBackend::onRespondChatroomInfoRequest(ChatroomInfo chatroomInfo, bool isManager)
{
  if (isManager)
    chatroomInfo.setManager(m_routableUserDiscoveryPrefix.getPrefix(IDENTITY_OFFSET));
  Name::Component chatroomName = chatroomInfo.getName();
  m_chatroomList[chatroomName].chatroomName = chatroomName.toUri();
  m_chatroomList[chatroomName].isManager = isManager;
  m_chatroomList[chatroomName].count = 0;
  m_chatroomList[chatroomName].info = chatroomInfo;
  sendChatroomList();
  onAddInRoster(m_routableUserDiscoveryPrefix.getPrefix(IDENTITY_OFFSET), chatroomName);
}

void
ChatroomDiscoveryBackend::onIdentityUpdated(const QString& identity)
{
  m_chatroomList.clear();
  m_identity = Name(identity.toStdString());
  m_userDiscoveryPrefix.clear();
  m_userDiscoveryPrefix.append(m_identity).append("CHRONOCHAT-DISCOVERYDATA");
  updatePrefixes();

  {
    std::lock_guard<std::mutex>lock(m_resumeMutex);
    m_shouldResume = true;
  }

  m_face->getIoService().stop();
}

void
ChatroomDiscoveryBackend::sendChatroomList()
{
  QStringList chatroomList;
  for (const auto& chatroom : m_chatroomList) {
    chatroomList << QString::fromStdString(chatroom.first.toUri());
  }

  emit chatroomListReady(chatroomList);
  if (m_refreshPanelId != nullptr) {
    m_scheduler->cancelEvent(m_refreshPanelId);
  }
  m_refreshPanelId = m_scheduler->scheduleEvent(REFRESH_INTERVAL,
                                                [this] { sendChatroomList(); });
}

void
ChatroomDiscoveryBackend::onWaitForChatroomInfo(const QString& chatroomName)
{
  auto chatroom = m_chatroomList.find(Name::Component(chatroomName.toStdString()));
  if (chatroom != m_chatroomList.end())
    emit chatroomInfoReady(chatroom->second.info, chatroom->second.isParticipant);
}

void
ChatroomDiscoveryBackend::shutdown()
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

  m_face->getIoService().stop();
}

void
ChatroomDiscoveryBackend::onNfdReconnect()
{
  std::lock_guard<std::mutex>lock(m_nfdConnectionMutex);
  m_isNfdConnected = true;
}

} // namespace chronochat

#if WAF
#include "chatroom-discovery-backend.moc"
#endif
