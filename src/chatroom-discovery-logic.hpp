/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Mengjin Yan <jane.yan0129@gmail.com>
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_CHATROOM_DISCOVERY_LOGIC_HPP
#define CHRONOCHAT_CHATROOM_DISCOVERY_LOGIC_HPP

#include "chatroom-info.hpp"
#include <ndn-cxx/util/scheduler.hpp>

namespace chronos {

class ChatroomDiscoveryLogic : noncopyable
{
public:
  typedef function<void(const ChatroomInfo& chatroomName, bool isAdd)> UpdateCallback;

  typedef std::map<Name::Component, chronos::ChatroomInfo> Chatrooms;
  static const size_t OFFSET_CHATROOM_NAME;
  static const size_t DISCOVERY_INTEREST_NAME_SIZE;
  static const size_t REFRESHING_INTEREST_NAME_SIZE;
  static const time::milliseconds DEFAULT_REFRESHING_TIMER;
  static const Name DISCOVERY_PREFIX;

public:
  explicit
  ChatroomDiscoveryLogic(shared_ptr<ndn::Face> face,
                         const UpdateCallback& updateCallback);


  /** \brief obtain the ongoing chatroom list
   */
  const Chatrooms&
  getChatrooms() const;

  /** \brief add a local chatroom in the ongoing chatroom list
   */
  void
  addLocalChatroom(const ChatroomInfo& chatroom);

  void
  removeLocalChatroom(const Name::Component& chatroomName);

  /** \brief send discovery interest
   */
  void
  sendDiscoveryInterest();

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  /**
   */
  void
  onDiscoveryInterest(const Name& name, const Interest& interest);

  /**
   */
  void
  onPrefixRegistrationFailed(const Name& name);

  /** \brief schedule another discovery
   */
  void
  onDiscoveryInterestTimeout(const Interest& interest);

  /** \brief send interest to find if the certain chatroom exists
   */
  void
  refreshChatroom(const Name::Component& chatroomName);

  /** \brief copy the contact from the participants of the chatroom to contacts of the chatroom
   */
  void
  addContacts(ChatroomInfo& chatroom);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:

  /** \brief erase the chatroom from the ongoing chatroom list
   */
  void
  onRefreshingInterestTimeout(const Interest& interest);

  /** \brief operate the chatroom data and schedule discovery and refreshing process
   */
  void
  onReceiveData(const ndn::Interest& interest, const ndn::Data& data, const bool isRefreshing);

private:
  static const time::seconds m_discoveryInterval;

  shared_ptr<ndn::Face> m_face;

  ndn::KeyChain m_keyChain;

  Chatrooms m_chatrooms;
  Chatrooms m_localChatrooms;

  ndn::Scheduler m_scheduler;

  UpdateCallback m_onUpdate;

};

inline const ChatroomDiscoveryLogic::Chatrooms&
ChatroomDiscoveryLogic::getChatrooms() const
{
  return m_chatrooms;
}

} // namespace chronos

#endif // CHRONOCHAT_CHATROOM_DISCOVERY_LOGIC_HPP
