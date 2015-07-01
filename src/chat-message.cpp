/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#include "chat-message.hpp"

namespace chronochat {

BOOST_CONCEPT_ASSERT((ndn::WireEncodable<ChatMessage>));
BOOST_CONCEPT_ASSERT((ndn::WireDecodable<ChatMessage>));

ChatMessage::ChatMessage()
{
}

ChatMessage::ChatMessage(const Block& chatMsgWire)
{
  this->wireDecode(chatMsgWire);
}

template<bool T>
size_t
ChatMessage::wireEncode(ndn::EncodingImpl<T>& encoder) const
{
  // ChatMessage := CHAT-MESSAGE-TYPE TLV-LENGTH
  //                  Nick
  //                  ChatroomName
  //                  ChatMessageType
  //                  ChatData
  //                  Timestamp
  //
  // Nick := NICK-NAME-TYPE TLV-LENGTH
  //           String
  //
  // ChatroomName := CHATROOM-NAME-TYPE TLV-LENGTH
  //                   String
  //
  // ChatMessageType := CHAT-MESSAGE-TYPE TLV-LENGTH
  //                      nonNegativeInteger
  //
  // ChatData := CHAT-DATA-TYPE TLV-LENGTH
  //               String
  //
  // Timestamp := TIMESTAMP-TYPE TLV-LENGTH
  //                VarNumber
  //
  size_t totalLength = 0;

  // Timestamp
  totalLength += prependNonNegativeIntegerBlock(encoder, tlv::Timestamp, m_timestamp);

  // ChatData
  if (m_msgType == CHAT) {
    const uint8_t* dataWire = reinterpret_cast<const uint8_t*>(m_data.c_str());
    totalLength += encoder.prependByteArrayBlock(tlv::ChatData, dataWire, m_data.length());
  }

  // ChatMessageType
  totalLength += prependNonNegativeIntegerBlock(encoder, tlv::ChatMessageType, m_msgType);

  // ChatroomName
  const uint8_t* chatroomWire = reinterpret_cast<const uint8_t*>(m_chatroomName.c_str());
  totalLength += encoder.prependByteArrayBlock(tlv::ChatroomName, chatroomWire,
                                               m_chatroomName.length());

  // Nick
  const uint8_t* nickWire = reinterpret_cast<const uint8_t*>(m_nick.c_str());
  totalLength += encoder.prependByteArrayBlock(tlv::Nick, nickWire, m_nick.length());

  // Chat Message
  totalLength += encoder.prependVarNumber(totalLength);
  totalLength += encoder.prependVarNumber(tlv::ChatMessage);

  return totalLength;
}

const Block&
ChatMessage::wireEncode() const
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
ChatMessage::wireDecode(const Block& chatMsgWire)
{
  m_wire = chatMsgWire;
  m_wire.parse();

  if (m_wire.type() != tlv::ChatMessage)
    throw Error("Unexpected TLV number when decoding chat message packet");

  Block::element_const_iterator i = m_wire.elements_begin();
  if (i == m_wire.elements_end() || i->type() != tlv::Nick)
    throw Error("Expect Nick but get ...");
  m_nick = std::string(reinterpret_cast<const char* >(i->value()),
                       i->value_size());;
  i++;

  if (i == m_wire.elements_end() || i->type() != tlv::ChatroomName)
    throw Error("Expect Chatroom Name but get ...");
  m_chatroomName = std::string(reinterpret_cast<const char* >(i->value()),
                       i->value_size());;
  i++;

  if (i == m_wire.elements_end() || i->type() != tlv::ChatMessageType)
    throw Error("Expect Chat Message Type but get ...");
  m_msgType = static_cast<ChatMessageType>(readNonNegativeInteger(*i));
  i++;

  if (m_msgType != CHAT)
    m_data = "";
  else {
    if (i == m_wire.elements_end() || i->type() != tlv::ChatData)
      throw Error("Expect Chat Data but get ...");
    m_data = std::string(reinterpret_cast<const char* >(i->value()),
                       i->value_size());;
    i++;
  }

  if (i == m_wire.elements_end() || i->type() != tlv::Timestamp)
    throw Error("Expect Timestamp but get ...");
  m_timestamp = static_cast<time_t>(readNonNegativeInteger(*i));
  i++;

  if (i != m_wire.elements_end()) {
    throw Error("Unexpected element");
  }

}

void
ChatMessage::setNick(const std::string& nick)
{
  m_wire.reset();
  m_nick = nick;
}

void
ChatMessage::setChatroomName(const std::string& chatroomName)
{
  m_wire.reset();
  m_chatroomName = chatroomName;
}

void
ChatMessage::setMsgType(const ChatMessageType msgType)
{
  m_wire.reset();
  m_msgType = msgType;
}

void
ChatMessage::setData(const std::string& data)
{
  m_wire.reset();
  m_data = data;
}

void
ChatMessage::setTimestamp(const time_t timestamp)
{
  m_wire.reset();
  m_timestamp = timestamp;
}

}// namespace chronochat
