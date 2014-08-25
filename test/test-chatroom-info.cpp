#include "chatroom-info.hpp"
#include <boost/test/unit_test.hpp>
#include <ndn-cxx/encoding/block.hpp>

namespace chronos {

namespace test {

using std::string;

BOOST_AUTO_TEST_SUITE(TestChatroomInfo)

const uint8_t chatroomInfo[] = {
  0x81, 0x2d, // ChatroomInfo
    0x82, 0x01, // TrustModel
      0x01,
    0x80, 0x14,// Participant1
      0x07, 0x12,
        0x08, 0x03,
          0x6e, 0x64, 0x6e,
        0x08, 0x04,
          0x75, 0x63, 0x6c, 0x61,
        0x08, 0x05,
          0x61, 0x6c, 0x69, 0x63, 0x65,
    0x80, 0x12, // Participant2
      0x07, 0x10,
        0x08, 0x03,
          0x6e, 0x64, 0x6e,
        0x08, 0x04,
          0x75, 0x63, 0x6c, 0x61,
        0x08, 0x03,
          0x79, 0x6d, 0x6a
};

BOOST_AUTO_TEST_CASE(EncodeChatroom)
{
  //Chatroom := CHATROOM-TYPE TLV-LENGTH
  //              TrustModel
  //              Participant+

  ChatroomInfo chatroom;
  chatroom.setName(ndn::Name::Component("lunch-talk"));
  chatroom.addParticipant(Name("/ndn/ucla/alice"));
  chatroom.addParticipant(Name("/ndn/ucla/ymj"));
  chatroom.setTrustModel(ChatroomInfo::TRUST_MODEL_WEBOFTRUST);

  const Block& encoded = chatroom.wireEncode();
  Block chatroomInfoBlock(chatroomInfo, sizeof(chatroomInfo));

  BOOST_CHECK_EQUAL_COLLECTIONS(chatroomInfoBlock.wire(),
                                chatroomInfoBlock.wire() + chatroomInfoBlock.size(),
                                encoded.wire(),
                                encoded.wire() + encoded.size());
}

BOOST_AUTO_TEST_CASE(DecodeChatroomCorrect)
{
  ChatroomInfo chatroom;
  chatroom.setName(ndn::Name::Component("lunch-talk"));
  chatroom.addParticipant(Name("/ndn/ucla/alice"));
  chatroom.addParticipant(Name("/ndn/ucla/ymj"));
  chatroom.setTrustModel(ChatroomInfo::TRUST_MODEL_WEBOFTRUST);

  Block chatroomInfoBlock(chatroomInfo, sizeof(chatroomInfo));
  ChatroomInfo dechatroom;
  dechatroom.wireDecode(chatroomInfoBlock);
  dechatroom.setName(ndn::Name::Component("lunch-talk"));

  BOOST_CHECK_EQUAL(chatroom.getName(), dechatroom.getName());
  BOOST_CHECK_EQUAL(chatroom.getParticipants().size(), dechatroom.getParticipants().size());
  BOOST_CHECK_EQUAL(chatroom.getParticipants()[0].toUri(), dechatroom.getParticipants()[0].toUri());
  BOOST_CHECK_EQUAL(chatroom.getParticipants()[1].toUri(), dechatroom.getParticipants()[1].toUri());
  BOOST_CHECK_EQUAL(chatroom.getTrustModel(), dechatroom.getTrustModel());
}

BOOST_AUTO_TEST_CASE(DecodeChatroomError)
{
  const uint8_t error1[] = {
    0x80, 0x2d, // Wrong ChatroomInfo Type (0x81, 0x2d)
      0x82, 0x01, // TrustModel
        0x01,
      0x80, 0x14,// Participant1
        0x07, 0x12,
          0x08, 0x03,
            0x6e, 0x64, 0x6e,
          0x08, 0x04,
            0x75, 0x63, 0x6c, 0x61,
          0x08, 0x05,
            0x61, 0x6c, 0x69, 0x63, 0x65,
      0x80, 0x12, // Participant2
        0x07, 0x10,
          0x08, 0x03,
            0x6e, 0x64, 0x6e,
          0x08, 0x04,
            0x75, 0x63, 0x6c, 0x61,
          0x08, 0x03,
            0x79, 0x6d, 0x6a
  };

  Block errorBlock1(error1, sizeof(error1));
  BOOST_CHECK_THROW(ChatroomInfo chatroom(errorBlock1), ChatroomInfo::Error);

  const uint8_t error2[] = {
    0x81, 0x2d, // ChatroomInfo
      0x81, 0x01, // Wrong TrustModel Type (0x82, 0x01)
        0x01,
      0x80, 0x14,// Participant1
        0x07, 0x12,
          0x08, 0x03,
            0x6e, 0x64, 0x6e,
          0x08, 0x04,
            0x75, 0x63, 0x6c, 0x61,
          0x08, 0x05,
            0x61, 0x6c, 0x69, 0x63, 0x65,
      0x80, 0x12, // Participant2
        0x07, 0x10,
          0x08, 0x03,
            0x6e, 0x64, 0x6e,
          0x08, 0x04,
            0x75, 0x63, 0x6c, 0x61,
          0x08, 0x03,
            0x79, 0x6d, 0x6a
  };

  Block errorBlock2(error2, sizeof(error2));
  BOOST_CHECK_THROW(ChatroomInfo chatroom(errorBlock2), ChatroomInfo::Error);

  const uint8_t error3[] = {
    0x81, 0x2d, // ChatroomInfo
      0x82, 0x01, // TrustModel
        0x01,
      0x80, 0x14,// Participant1
        0x07, 0x12,
          0x08, 0x03,
            0x6e, 0x64, 0x6e,
          0x08, 0x04,
            0x75, 0x63, 0x6c, 0x61,
          0x08, 0x05,
            0x61, 0x6c, 0x69, 0x63, 0x65,
      0x81, 0x12, // Wrong Participant Type (0x80, 0x12)
        0x07, 0x10,
          0x08, 0x03,
            0x6e, 0x64, 0x6e,
          0x08, 0x04,
            0x75, 0x63, 0x6c, 0x61,
          0x08, 0x03,
            0x79, 0x6d, 0x6a
  };

  Block errorBlock3(error3, sizeof(error3));
  BOOST_CHECK_THROW(ChatroomInfo chatroom(errorBlock3), ChatroomInfo::Error);

  const uint8_t error4[] = {
    0x81, 0x00 // Empty ChatroomInfo
  };

  Block errorBlock4(error4, sizeof(error4));
  BOOST_CHECK_THROW(ChatroomInfo chatroom(errorBlock4), ChatroomInfo::Error);

  const uint8_t error5[] = {
    0x81, 0x03, // ChatroomInfo
      0x82, 0x01, // TrustModel
        0x01
    //zero Participant
  };

  Block errorBlock5(error5, sizeof(error5));
  BOOST_CHECK_THROW(ChatroomInfo chatroom(errorBlock5), ChatroomInfo::Error);
}

BOOST_AUTO_TEST_SUITE_END()

} //namespace test

} //namespace chronos
