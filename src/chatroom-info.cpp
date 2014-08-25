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

namespace chronos {

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

  //Chatroom := CHATROOM-TYPE TLV-LENGTH
  //              TrustModel
  //              Participant+

  //Participants
  for (std::vector<Name>::const_reverse_iterator it = m_participants.rbegin();
       it != m_participants.rend(); ++it) {
    size_t entryLength = 0;

    entryLength += it->wireEncode(block);
    entryLength += block.prependVarNumber(entryLength);
    entryLength += block.prependVarNumber(tlv::PARTICIPANT);
    totalLength += entryLength;
  }

  //TrustModel
  totalLength += prependNonNegativeIntegerBlock(block, tlv::TRUSTMODEL, m_trustModel);

  //type = TYPE_CHATROOM;
  totalLength += block.prependVarNumber(totalLength);
  totalLength += block.prependVarNumber(tlv::CHATROOM);

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

  //Chatroom := CHATROOM-TYPE TLV-LENGTH
  //              TrustModel
  //              Participant+

  if (m_wire.type() != tlv::CHATROOM)
    throw Error("Unexpected TLV number when decoding chatroom packet");

  Block::element_const_iterator i = m_wire.elements_begin();

  //TrustModel
  if (i == m_wire.elements_end() || i->type() != tlv::TRUSTMODEL)
    throw Error("Missing TrustModel");
  m_trustModel =
      static_cast<TrustModel>(readNonNegativeInteger(*i));

  ++i;

  //Participants
  for (; i != m_wire.elements_end() && i->type() == tlv::PARTICIPANT; ++i) {
    Name name;
    name.wireDecode(i->blockFromValue());
    m_participants.push_back(name);
  }
  if (m_participants.empty())
    throw Error("Missing Participant");
  if (i != m_wire.elements_end()) {
    throw Error("Unexpected element");
  }
}

void
ChatroomInfo::addParticipant(const Name& participant)
{
  m_wire.reset();
  m_participants.push_back(participant);
}

void
ChatroomInfo::addContact(const Name& contact)
{
  m_wire.reset();
  m_contacts.push_back(contact);
}

void
ChatroomInfo::setName(const Name::Component& name)
{
  m_wire.reset();
  m_name = name;
}

void
ChatroomInfo::setTrustModel(const TrustModel trustModel)
{
  m_wire.reset();
  m_trustModel = trustModel;
}

} //namespace chronos
