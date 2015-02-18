/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#include <boost/test/unit_test.hpp>

#include "chat-message.hpp"
#include <ndn-cxx/encoding/buffer-stream.hpp>

namespace chronochat{
namespace tests{

using std::string;

BOOST_AUTO_TEST_SUITE(TestChatMessage)

BOOST_AUTO_TEST_CASE(EncodeDecode)
{
  string nick("qiuhan");
  string chatroomName("test");
  int32_t seconds =
    static_cast<int32_t>(time::toUnixTimestamp(time::system_clock::now()).count() / 1000);
  ChatMessage helloMsg;
  helloMsg.setNick(nick);
  helloMsg.setChatroomName(chatroomName);
  helloMsg.setTimestamp(seconds);
  helloMsg.setMsgType(ChatMessage::ChatMessageType::HELLO);

  Block helloWire;
  BOOST_REQUIRE_NO_THROW(helloWire = helloMsg.wireEncode());
  ChatMessage decodedHelloMsg;
  BOOST_REQUIRE_NO_THROW(decodedHelloMsg.wireDecode(helloWire));

  BOOST_CHECK_EQUAL(decodedHelloMsg.getNick(), nick);
  BOOST_CHECK_EQUAL(decodedHelloMsg.getChatroomName(), chatroomName);
  BOOST_CHECK_EQUAL(decodedHelloMsg.getTimestamp(), seconds);
  BOOST_CHECK_EQUAL(decodedHelloMsg.getMsgType(), ChatMessage::ChatMessageType::HELLO);

  ChatMessage chatMsg;
  string data = "This is for testing";
  chatMsg.setNick(nick);
  chatMsg.setChatroomName(chatroomName);
  chatMsg.setTimestamp(seconds);
  chatMsg.setData(data);
  chatMsg.setMsgType(ChatMessage::ChatMessageType::CHAT);

  Block chatWire;
  BOOST_REQUIRE_NO_THROW(chatWire = chatMsg.wireEncode());
  ChatMessage decodedChatMsg;
  BOOST_REQUIRE_NO_THROW(decodedChatMsg.wireDecode(chatWire));

  BOOST_CHECK_EQUAL(decodedChatMsg.getNick(), nick);
  BOOST_CHECK_EQUAL(decodedChatMsg.getChatroomName(), chatroomName);
  BOOST_CHECK_EQUAL(decodedChatMsg.getTimestamp(), seconds);
  BOOST_CHECK_EQUAL(decodedChatMsg.getData(), data);
  BOOST_CHECK_EQUAL(decodedChatMsg.getMsgType(), ChatMessage::ChatMessageType::CHAT);

}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace chronochat
