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
#include "chatroom-info.hpp"

namespace chronochat {

ChatroomInfo::ChatroomInfo()
{
}

ChatroomInfo::ChatroomInfo(const Block& chatroomWire)
{
  this->wireDecode(chatroomWire);
}

template<bool T>
size_t
ChatroomInfo::wireEncode(ndn::EncodingImpl<T>& block) const
{
  size_t totalLength = 0;

  // ChatroomInfo := CHATROOM-INFO-TYPE TLV-LENGTH
  //                   ChatroomName
  //                   TrustModel
  //                   ChatroomPrefix
  //                   ManagerPrefix
  //                   Participants
  //
  // ChatroomName := CHATROOM-NAME-TYPE TLV-LENGTH
  //                   NameComponent
  //
  // TrustModel := TRUST-MODEL-TYPE TLV-LENGTH
  //                 nonNegativeInteger
  //
  // ChatroomPrefix := CHATROOM-PREFIX-TYPE TLV-LENGTH
  //                     Name
  //
  // ManagerPrefix := MANAGER-PREFIX-TYPE TLV-LENGTH
  //                    Name
  //
  // Participants := PARTICIPANTS-TYPE TLV-LENGTH
  //                   Name+

  // Participants
  size_t participantsLength = 0;
  for (std::list<Name>::const_reverse_iterator it = m_participants.rbegin();
       it != m_participants.rend(); ++it) {
    participantsLength += it->wireEncode(block);
  }
  participantsLength += block.prependVarNumber(participantsLength);
  participantsLength += block.prependVarNumber(tlv::Participants);
  totalLength += participantsLength;

  // Manager Prefix
  size_t managerLength = m_manager.wireEncode(block);
  totalLength += managerLength;
  totalLength += block.prependVarNumber(managerLength);
  totalLength += block.prependVarNumber(tlv::ManagerPrefix);

  // Chatroom Sync Prefix
  size_t chatroomSyncPrefixLength = m_syncPrefix.wireEncode(block);
  totalLength += chatroomSyncPrefixLength;
  totalLength += block.prependVarNumber(chatroomSyncPrefixLength);
  totalLength += block.prependVarNumber(tlv::ChatroomPrefix);

  // Trust Model
  totalLength += ndn::prependNonNegativeIntegerBlock(block, tlv::TrustModel, m_trustModel);

  // Chatroom Name
  size_t chatroomNameLength = m_chatroomName.wireEncode(block);
  totalLength += chatroomNameLength;
  totalLength += block.prependVarNumber(chatroomNameLength);
  totalLength += block.prependVarNumber(tlv::ChatroomName);

  // Chatroom Info
  totalLength += block.prependVarNumber(totalLength);
  totalLength += block.prependVarNumber(tlv::ChatroomInfo);

  return totalLength;
}

const Block&
ChatroomInfo::wireEncode() const
{
  ndn::EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator);

  ndn::EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer);

  m_wire = buffer.block();
  m_wire.parse();

  return m_wire;
}

void
ChatroomInfo::wireDecode(const Block& chatroomWire)
{
  m_wire = chatroomWire;
  m_wire.parse();

  m_participants.clear();

  // ChatroomInfo := CHATROOM-INFO-TYPE TLV-LENGTH
  //                   ChatroomName
  //                   TrustModel
  //                   ChatroomPrefix
  //                   ManagerPrefix
  //                   Participants
  //
  // ChatroomName := CHATROOM-NAME-TYPE TLV-LENGTH
  //                   NameComponent
  //
  // TrustModel := TRUST-MODEL-TYPE TLV-LENGTH
  //                 nonNegativeInteger
  //
  // ChatroomPrefix := CHATROOM-PREFIX-TYPE TLV-LENGTH
  //                     Name
  //
  // ManagerPrefix := MANAGER-PREFIX-TYPE TLV-LENGTH
  //                    Name
  //
  // Participants := PARTICIPANTS-TYPE TLV-LENGTH
  //                   Name+

  if (m_wire.type() != tlv::ChatroomInfo)
    throw Error("Unexpected TLV number when decoding chatroom packet");

  // Chatroom Info
  Block::element_const_iterator i = m_wire.elements_begin();
  if (i == m_wire.elements_end() || i->type() != tlv::ChatroomName)
    throw Error("Missing Chatroom Name Info");
  m_chatroomName.wireDecode(i->blockFromValue());
  ++i;

  // Trust Model
  if (i == m_wire.elements_end() || i->type() != tlv::TrustModel)
    throw Error("Missing TrustModel");
  m_trustModel =
      static_cast<TrustModel>(readNonNegativeInteger(*i));
  ++i;

  // Chatroom Sync Prefix
  if (i == m_wire.elements_end() || i->type() != tlv::ChatroomPrefix)
    throw Error("Missing Chatroom Prefix");
  m_syncPrefix.wireDecode(i->blockFromValue());
  ++i;

  // Manager Prefix
  if (i == m_wire.elements_end() || i->type() != tlv::ManagerPrefix)
    throw Error("Missing Manager Prefix");
  m_manager.wireDecode(i->blockFromValue());
  ++i;

  // Participants
  if (i == m_wire.elements_end() || i->type() != tlv::Participants)
    throw Error("Missing Participant");

  Block temp = *i;
  temp.parse();

  Block::element_const_iterator j = temp.elements_begin();

  while (j != temp.elements_end() && j->type() == tlv::Name) {
    m_participants.push_back(Name(*j));
    ++j;
  }
  if (j != temp.elements_end())
    throw Error("Unexpected element");

  if (m_participants.empty())
    throw Error("Missing Participant");

  ++i;

  if (i != m_wire.elements_end()) {
    throw Error("Unexpected element");
  }
}

void
ChatroomInfo::setName(const Name::Component& name)
{
  m_wire.reset();
  m_chatroomName = name;
}

void
ChatroomInfo::setTrustModel(const TrustModel trustModel)
{
  m_wire.reset();
  m_trustModel = trustModel;
}

void
ChatroomInfo::addParticipant(const Name& participant)
{
  m_wire.reset();
  if (find(m_participants.begin(), m_participants.end(), participant) ==
      m_participants.end()) {
    m_participants.push_back(participant);
  }
}

void
ChatroomInfo::removeParticipant(const Name& participant)
{
  m_wire.reset();
  m_participants.remove(participant);
}

void
ChatroomInfo::setSyncPrefix(const Name& prefix)
{
  m_wire.reset();
  m_syncPrefix = prefix;
}

void
ChatroomInfo::setManager(const Name& manager)
{
  m_wire.reset();
  m_manager = manager;
}

} // namespace chronochat
