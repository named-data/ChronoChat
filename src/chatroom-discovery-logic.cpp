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

#include "chatroom-discovery-logic.hpp"


namespace chronos {

const size_t ChatroomDiscoveryLogic::OFFSET_CHATROOM_NAME = 4;
const size_t ChatroomDiscoveryLogic::DISCOVERY_INTEREST_NAME_SIZE = 4;
const size_t ChatroomDiscoveryLogic::REFRESHING_INTEREST_NAME_SIZE = 5;
const time::milliseconds ChatroomDiscoveryLogic::DEFAULT_REFRESHING_TIMER(10000);
const Name ChatroomDiscoveryLogic::DISCOVERY_PREFIX("/ndn/broadcast/chronochat/chatroom-list");
const time::seconds ChatroomDiscoveryLogic::m_discoveryInterval(600);

ChatroomDiscoveryLogic::ChatroomDiscoveryLogic(shared_ptr<ndn::Face> face,
                                               const UpdateCallback& updateCallback)
  : m_face(face)
  , m_scheduler(face->getIoService())
  , m_onUpdate(updateCallback)
{

  m_face->setInterestFilter(DISCOVERY_PREFIX,
                            bind(&ChatroomDiscoveryLogic::onDiscoveryInterest,
                                 this, _1, _2),
                            bind(&ChatroomDiscoveryLogic::onPrefixRegistrationFailed,
                                 this, _1));
}

void
ChatroomDiscoveryLogic::addLocalChatroom(const ChatroomInfo& chatroom)
{
  m_localChatrooms[chatroom.getName()] = chatroom;
  m_chatrooms[chatroom.getName()] = chatroom; // no need to discover local chatroom
}

void
ChatroomDiscoveryLogic::removeLocalChatroom(const Name::Component& chatroomName)
{
  m_localChatrooms.erase(chatroomName);
  m_chatrooms.erase(chatroomName);
}

void
ChatroomDiscoveryLogic::sendDiscoveryInterest()
{
  Interest discovery(DISCOVERY_PREFIX);
  discovery.setMustBeFresh(true);
  discovery.setInterestLifetime(time::milliseconds(10000));

  Exclude exclude;
  Chatrooms::iterator it;
  for (it = m_chatrooms.begin(); it != m_chatrooms.end(); ++it) {
    exclude.excludeOne(it->first);
  }
  discovery.setExclude(exclude);

  m_face->expressInterest(discovery,
                          bind(&ChatroomDiscoveryLogic::onReceiveData, this,
                               _1, _2, false),
                          bind(&ChatroomDiscoveryLogic::onDiscoveryInterestTimeout, this,
                               _1));

}

void
ChatroomDiscoveryLogic::onDiscoveryInterest(const Name& name, const Interest& interest)
{
  if (m_localChatrooms.empty())
    return;

  if (interest.getName() == DISCOVERY_PREFIX) {
    // discovery
    for (Chatrooms::const_iterator it = m_localChatrooms.begin();
         it != m_localChatrooms.end(); ++it) {

      if (!interest.getExclude().empty() &&
          interest.getExclude().isExcluded(it->first))
        continue;


      Name dataName(interest.getName());
      dataName.append(it->first);

      shared_ptr<Data> chatroomInfo = make_shared<Data>(dataName);
      chatroomInfo->setFreshnessPeriod(time::seconds(10));
      chatroomInfo->setContent(it->second.wireEncode());

      m_keyChain.sign(*chatroomInfo);
      m_face->put(*chatroomInfo);
      break;
    }
    return;
  }

  if (DISCOVERY_PREFIX.isPrefixOf(interest.getName()) &&
      interest.getName().size() == REFRESHING_INTEREST_NAME_SIZE) {
    // refreshing
    Chatrooms::const_iterator it =
      m_localChatrooms.find(interest.getName().get(OFFSET_CHATROOM_NAME));

    if (it == m_localChatrooms.end())
      return;

    shared_ptr<Data> chatroomInfo = make_shared<Data>(interest.getName());
    chatroomInfo->setFreshnessPeriod(time::seconds(10));
    chatroomInfo->setContent(it->second.wireEncode());

    m_keyChain.sign(*chatroomInfo);
    m_face->put(*chatroomInfo);

    return;
  }
}

void
ChatroomDiscoveryLogic::onPrefixRegistrationFailed(const Name& name)
{
}

void
ChatroomDiscoveryLogic::refreshChatroom(const Name::Component& chatroomName)
{
  Name name = DISCOVERY_PREFIX;
  name.append(chatroomName);

  BOOST_ASSERT(name.size() == REFRESHING_INTEREST_NAME_SIZE);

  Interest discovery(name);
  discovery.setMustBeFresh(true);
  discovery.setInterestLifetime(time::milliseconds(10000));

  m_face->expressInterest(discovery,
                          bind(&ChatroomDiscoveryLogic::onReceiveData, this,
                               _1, _2, true),
                          bind(&ChatroomDiscoveryLogic::onRefreshingInterestTimeout, this,
                               _1));

}

void
ChatroomDiscoveryLogic::onReceiveData(const ndn::Interest& interest,
                                      const ndn::Data& data,
                                      const bool isRefreshing)
{
  Name::Component chatroomName = data.getName().get(OFFSET_CHATROOM_NAME);

  ChatroomInfo chatroom;
  chatroom.wireDecode(data.getContent().blockFromValue());
  chatroom.setName(chatroomName);

  // Tmp Disabled
  // if (chatroom.getTrustModel() == ChatroomInfo::TRUST_MODEL_WEBOFTRUST)
  //   addContacts(chatroom);


  m_chatrooms[chatroomName] = chatroom;
  m_onUpdate(chatroom, true); //add

  time::milliseconds refreshingTime;
  if (data.getFreshnessPeriod() > DEFAULT_REFRESHING_TIMER)
    refreshingTime = data.getFreshnessPeriod();
  else
    refreshingTime = DEFAULT_REFRESHING_TIMER;

  m_scheduler.scheduleEvent(refreshingTime,
                            bind(&ChatroomDiscoveryLogic::refreshChatroom, this, chatroomName));

  if (!isRefreshing)
    sendDiscoveryInterest();
}

void
ChatroomDiscoveryLogic::onRefreshingInterestTimeout(const ndn::Interest& interest)
{
  Name::Component chatroomName = interest.getName().get(OFFSET_CHATROOM_NAME);

  Chatrooms::iterator it = m_chatrooms.find(chatroomName);
  if (it != m_chatrooms.end()) {
    m_onUpdate(it->second, false); //delete
    m_chatrooms.erase(it);
  }
}


void
ChatroomDiscoveryLogic::onDiscoveryInterestTimeout(const Interest& interest)
{
  m_scheduler.scheduleEvent(m_discoveryInterval,
                            bind(&ChatroomDiscoveryLogic::sendDiscoveryInterest, this));
}

void
ChatroomDiscoveryLogic::addContacts(ChatroomInfo& chatroom)
{
  // Tmp Disabled

  // std::vector<Name>::const_iterator nameIt;
  // std::vector<shared_ptr<Contact> >::iterator contactIt;
  // ContactList contactList;
  // m_contactManager->getContactList(contactList);

  // for (contactIt = contactList.begin();
  //      contactIt != contactList.end(); ++contactIt) {
  //   nameIt = std::find(chatroom.getParticipants().begin(),
  //                 chatroom.getParticipants().end(), (*contactIt)->getNameSpace());
  //   if (nameIt != chatroom.getParticipants().end())
  //     chatroom.addContact(*nameIt);
  // }
}


} //namespace chronos
