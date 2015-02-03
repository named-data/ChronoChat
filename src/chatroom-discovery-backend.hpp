/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 *         Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_CHATROOM_DISCOVERY_BACKEND_HPP
#define CHRONOCHAT_CHATROOM_DISCOVERY_BACKEND_HPP

#include <QThread>

#ifndef Q_MOC_RUN
#include "common.hpp"
#include "chatroom-info.hpp"
#include <boost/random.hpp>
#include <mutex>
#include <socket.hpp>
#include <boost/thread.hpp>
#endif

namespace chronochat {

class ChatroomInfoBackend {
public:
  std::string chatroomName;
  Name chatroomPrefix;
  ChatroomInfo info;
  // For a chatroom's user to check whether his own chatroom is alive
  ndn::EventId localChatroomTimeoutEventId;
  // If the manager no longer exist, set a random timer to compete for manager
  ndn::EventId managerSelectionTimeoutEventId;
  // For a user to check the status of the chatroom that he is not in.
  ndn::EventId remoteChatroomTimeoutEventId;
  // If the user is manager, he will need the helloEventId to keep track of hello message
  ndn::EventId helloTimeoutEventId;
  // To tell whether the user is in this chatroom
  bool isParticipant;
  // To tell whether the user is the manager
  bool isManager;
  // Variable to tell whether another manager exists
  int count;
  // Variable to tell whether to print on the panel
  bool isPrint;

};

class ChatroomDiscoveryBackend : public QThread
{
  Q_OBJECT

public:
  ChatroomDiscoveryBackend(const Name& routingPrefix,
                           const Name& identity,
                           QObject* parent = nullptr);

  ~ChatroomDiscoveryBackend();


protected:
  void
  run();

private:
  void
  initializeSync();

  void
  close();

  void
  processSyncUpdate(const std::vector<chronosync::MissingDataInfo>& updates);

  void
  processChatroomData(const ndn::shared_ptr<const ndn::Data>& data);

  void
  localSessionTimeout(const Name::Component& chatroomName);

  void
  remoteSessionTimeout(const Name::Component& chatroomName);

  void
  randomSessionTimeout(const Name::Component& chatroomName);

  void
  sendUpdate(const Name::Component& chatroomName);

  void
  updatePrefixes();

  void
  sendChatroomList();

signals:
  /**
   * @brief request to get chatroom info
   *
   * This signal will be sent to controller to get chatroom info
   *
   * @param chatroomName specify which chatroom's info to request
   * @param isManager if the user who send the signal is the manager of the chatroom
   */
  void
  chatroomInfoRequest(std::string chatroomName, bool isManager);

  /**
   * @brief send chatroom list to front end
   *
   * @param chatroomList the list of chatrooms
   */
  void
  chatroomListReady(const QStringList& chatroomList);

  /**
   * @brief send chatroom info to front end
   *
   * @param info the chatroom info request by front end
   * @param isParticipant if the user is a participant of the chatroom
   */
  void
  chatroomInfoReady(const ChatroomInfo& info, bool isParticipant);

  void
  nfdError();

public slots:

  /**
   * @brief update routing prefix
   *
   * @param routingPrefix the new routing prefix
   */
  void
  updateRoutingPrefix(const QString& routingPrefix);

  /**
   * @brief erase a participant in a chatroom's roster
   *
   * This slot is called by chat dialog will erase a participant from a chatroom.
   * If the user is the manager of this chatroom, he will publish an update.
   *
   * @param sessionPrefix the prefix of the participant to erase
   * @param chatroomName the name of the chatroom
   */
  void
  onEraseInRoster(ndn::Name sessionPrefix, ndn::Name::Component chatroomName);

  /**
   * @brief add a participant in a chatroom's roster
   *
   * This slot is called by chat dialog and will add a participant from a chatroom.
   * If the user is the manager of this chatroom, he will publish an update.
   *
   * @param sessionPrefix the prefix of the participant to erase
   * @param chatroomName the name of the chatroom
   */
  void
  onAddInRoster(ndn::Name sessionPrefix, ndn::Name::Component chatroomName);

  /**
   * @brief is called when user himself join a chatroom
   *
   * This slot is called by controller and will modify the chatroom list in discovery
   *
   * @param chatroomName the name of chatroom the user join
   */
  void
  onNewChatroomForDiscovery(ndn::Name::Component chatroomName);

  /**
   * @brief get chatroom info from chat dialog
   *
   * This slot is called by controller. It get the chatroom info of a chatroom and
   * if the user is the manager, he will publish an update
   *
   * @param chatroomInfo chatroom info
   * @param isManager whether the user is the manager of the chatroom
   */
  void
  onRespondChatroomInfoRequest(ChatroomInfo chatroomInfo, bool isManager);

  /**
   * @brief reset when the identity updates
   *
   * This slot is called when the identity is updated, it will update the identity and
   * reset the socket
   *
   * @param identity new identity
   */
  void
  onIdentityUpdated(const QString& identity);

  /**
   * @brief prepare chatroom info for discovery panel
   *
   * This slot is called by discovery panel when it wants to display the info of a chatroom
   *
   * @param chatroomName the name of chatroom the discovery panel requested
   */
  void
  onWaitForChatroomInfo(const QString& chatroomName);

  void
  shutdown();

  void
  onNfdReconnect();

private:

  typedef std::map<ndn::Name::Component, ChatroomInfoBackend> ChatroomList;

  bool m_shouldResume;
  bool m_isNfdConnected;
  Name m_discoveryPrefix;
  Name m_routableUserDiscoveryPrefix;
  Name m_routingPrefix;
  Name m_userDiscoveryPrefix;
  Name m_identity;

  boost::mt19937 m_randomGenerator;
  boost::variate_generator<boost::mt19937&, boost::uniform_int<> > m_rangeUniformRandom;

  shared_ptr<ndn::Face> m_face;

  unique_ptr<ndn::Scheduler> m_scheduler;            // scheduler
  ndn::EventId m_refreshPanelId;
  shared_ptr<chronosync::Socket> m_sock; // SyncSocket

  ChatroomList m_chatroomList;
  std::mutex m_resumeMutex;
  std::mutex m_nfdConnectionMutex;

};

} // namespace chronochat

#endif // CHRONOCHAT_CHATROOM_DISCOVERY_BACKEND_HPP
