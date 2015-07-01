/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_CHAT_MESSAGE_HPP
#define CHRONOCHAT_CHAT_MESSAGE_HPP

#include "common.hpp"
#include "tlv.hpp"
#include <ndn-cxx/util/concepts.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/encoding-buffer.hpp>
#include <boost/concept_check.hpp>

namespace chronochat {

class ChatMessage
{

public:

  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  enum ChatMessageType {
    CHAT = 0,
    HELLO = 1,
    LEAVE = 2,
    JOIN = 3,
    OTHER = 4,
  };

public:

  ChatMessage();

  explicit
  ChatMessage(const Block& chatMsgWire);

  const Block&
  wireEncode() const;

  void
  wireDecode(const Block& chatMsgWire);

  const std::string&
  getNick() const;

  const std::string&
  getChatroomName() const;

  const ChatMessageType
  getMsgType() const;

  const std::string&
  getData() const;

  const time_t
  getTimestamp() const;

  void
  setNick(const std::string& nick);

  void
  setChatroomName(const std::string& chatroomName);

  void
  setMsgType(const ChatMessageType msgType);

  void
  setData(const std::string& data);

  void
  setTimestamp(const time_t timestamp);

private:
  template<bool T>
  size_t
  wireEncode(ndn::EncodingImpl<T>& encoder) const;

private:
  mutable Block m_wire;
  std::string m_nick;
  std::string m_chatroomName;
  ChatMessageType m_msgType;
  std::string m_data;
  time_t m_timestamp;

};

inline const std::string&
ChatMessage::getNick() const
{
  return m_nick;
}

inline const std::string&
ChatMessage::getChatroomName() const
{
  return m_chatroomName;
}

inline const ChatMessage::ChatMessageType
ChatMessage::getMsgType() const
{
  return m_msgType;
}

inline const std::string&
ChatMessage::getData() const
{
  return m_data;
}

inline const time_t
ChatMessage::getTimestamp() const
{
  return m_timestamp;
}

} // namespace chronochat

#endif //CHRONOCHAT_CHAT_MESSAGE_HPP
