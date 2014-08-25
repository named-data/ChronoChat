#include "chatroom-discovery-logic.hpp"
#include <boost/test/unit_test.hpp>
#include "dummy-client-face.hpp"

namespace chronos {

namespace test {

using std::string;

BOOST_AUTO_TEST_SUITE(TestChatroomDiscoveryLogic)

class Fixture
{
public:
  Fixture()
    : face(makeDummyClientFace())
  {
  }

  void
  update(const ChatroomInfo& chatroomName, bool isAdd)
  {
  }

public:
  shared_ptr<DummyClientFace> face;
};

BOOST_FIXTURE_TEST_CASE(AddLocalChatroom, Fixture)
{
  ChatroomDiscoveryLogic chatroomDiscoveryLogic(face, bind(&Fixture::update, this, _1, _2));

  ChatroomInfo chatroom;
  chatroom.setName(ndn::Name::Component("lunch-talk"));
  chatroom.addParticipant(Name("/ndn/ucla/ymj"));
  chatroom.addParticipant(Name("/ndn/ucla/alice"));
  chatroom.setTrustModel(ChatroomInfo::TRUST_MODEL_WEBOFTRUST);
  chatroomDiscoveryLogic.addLocalChatroom(chatroom);

  ndn::Name::Component chatroomName("lunch-talk");

  BOOST_CHECK_EQUAL(chatroomDiscoveryLogic.getChatrooms().size(), 1);
  BOOST_CHECK_EQUAL(chatroom.getName(),
                    chatroomDiscoveryLogic.getChatrooms().find(chatroomName)
                    ->second.getName());
  BOOST_CHECK_EQUAL(chatroom.getParticipants().size(),
                    chatroomDiscoveryLogic
                    .getChatrooms().find(chatroomName)->second.getParticipants().size());
  BOOST_CHECK_EQUAL(chatroom.getParticipants()[0].toUri(),
                    chatroomDiscoveryLogic
                    .getChatrooms().find(chatroomName)->second
                    .getParticipants()[0].toUri());
  BOOST_CHECK_EQUAL(chatroom.getParticipants()[1].toUri(),
                    chatroomDiscoveryLogic
                    .getChatrooms().find(chatroomName)->second
                    .getParticipants()[1].toUri());
  BOOST_CHECK_EQUAL(chatroom.getTrustModel(),
                    chatroomDiscoveryLogic
                    .getChatrooms().find(chatroomName)->second.getTrustModel());
}

BOOST_FIXTURE_TEST_CASE(RemoveLocalChatroom, Fixture)
{
  ChatroomDiscoveryLogic chatroomDiscoveryLogic(face, bind(&Fixture::update, this, _1, _2));

  ChatroomInfo chatroom;
  chatroom.setName(ndn::Name::Component("lunch-talk"));
  chatroom.addParticipant(Name("/ndn/ucla/ymj"));
  chatroom.addParticipant(Name("/ndn/ucla/alice"));
  chatroom.setTrustModel(ChatroomInfo::TRUST_MODEL_WEBOFTRUST);
  chatroomDiscoveryLogic.addLocalChatroom(chatroom);

  ndn::Name::Component chatroomName1("lunch-talk");
  ndn::Name::Component chatroomName2("supper-talk");

  chatroomDiscoveryLogic.removeLocalChatroom(chatroomName2);

  BOOST_CHECK_EQUAL(chatroomDiscoveryLogic.getChatrooms().size(), 1);
  BOOST_CHECK_EQUAL(chatroom.getName(),
                    chatroomDiscoveryLogic.getChatrooms().find(chatroomName1)
                    ->second.getName());
  BOOST_CHECK_EQUAL(chatroom.getParticipants().size(),
                    chatroomDiscoveryLogic
                    .getChatrooms().find(chatroomName1)->second.getParticipants().size());
  BOOST_CHECK_EQUAL(chatroom.getParticipants()[0].toUri(),
                    chatroomDiscoveryLogic
                    .getChatrooms().find(chatroomName1)->second
                    .getParticipants()[0].toUri());
  BOOST_CHECK_EQUAL(chatroom.getParticipants()[1].toUri(),
                    chatroomDiscoveryLogic
                    .getChatrooms().find(chatroomName1)->second
                    .getParticipants()[1].toUri());
  BOOST_CHECK_EQUAL(chatroom.getTrustModel(),
                    chatroomDiscoveryLogic
                    .getChatrooms().find(chatroomName1)->second.getTrustModel());

  chatroomDiscoveryLogic.removeLocalChatroom(chatroomName1);
  BOOST_CHECK_EQUAL(chatroomDiscoveryLogic.getChatrooms().size(), 0);
}

BOOST_FIXTURE_TEST_CASE(sendChatroomInterestFunction, Fixture)
{
  ChatroomDiscoveryLogic chatroomDiscoveryLogic(face, bind(&Fixture::update, this, _1, _2));
  chatroomDiscoveryLogic.sendDiscoveryInterest();

  face->processEvents(time::milliseconds(100));

  //with no exclude filter
  BOOST_CHECK_EQUAL(face->m_sentInterests.size(), 2);//first is nfd register interest
  BOOST_CHECK_EQUAL(face->m_sentDatas.size(), 0);
  BOOST_CHECK_EQUAL(face->m_sentInterests[1].getName().toUri(),
                    ChatroomDiscoveryLogic::DISCOVERY_PREFIX.toUri());
  BOOST_CHECK_EQUAL(face->m_sentInterests[1].getExclude().size(), 0);

  face->m_sentInterests.clear();

  //with exclude filter
  ChatroomInfo chatroom;
  chatroom.setName(ndn::Name::Component("lunch-talk"));
  chatroom.addParticipant(Name("/ndn/ucla/ymj"));
  chatroom.addParticipant(Name("/ndn/ucla/alice"));
  chatroom.setTrustModel(ChatroomInfo::TRUST_MODEL_WEBOFTRUST);
  chatroomDiscoveryLogic.addLocalChatroom(chatroom);

  chatroomDiscoveryLogic.sendDiscoveryInterest();
  face->processEvents(time::milliseconds(100));

  BOOST_CHECK_EQUAL(face->m_sentInterests.size(), 1);
  BOOST_CHECK_EQUAL(face->m_sentDatas.size(), 0);
  BOOST_CHECK_EQUAL(face->m_sentInterests[0].getName().toUri(),
                    ChatroomDiscoveryLogic::DISCOVERY_PREFIX.toUri());
  BOOST_CHECK_EQUAL(face->m_sentInterests[0].getExclude().size(), 1);
  BOOST_CHECK_EQUAL(face->m_sentInterests[0].getExclude().toUri(), "lunch-talk");
}

BOOST_FIXTURE_TEST_CASE(onDiscoveryInterest, Fixture)
{
  face->m_sentInterests.clear();
  face->m_sentDatas.clear();

  Interest discovery(ChatroomDiscoveryLogic::DISCOVERY_PREFIX);
  discovery.setMustBeFresh(true);
  discovery.setInterestLifetime(time::milliseconds(10000));

  Exclude exclude;
  exclude.excludeOne(ndn::Name::Component("lunch-talk"));
  discovery.setExclude(exclude);

  ChatroomDiscoveryLogic chatroomDiscoveryLogic(face, bind(&Fixture::update, this, _1, _2));

  ChatroomInfo chatroom1;
  chatroom1.setName(ndn::Name::Component("lunch-talk"));
  chatroom1.addParticipant(Name("/ndn/ucla/ymj"));
  chatroom1.addParticipant(Name("/ndn/ucla/alice"));
  chatroom1.setTrustModel(ChatroomInfo::TRUST_MODEL_WEBOFTRUST);
  chatroomDiscoveryLogic.addLocalChatroom(chatroom1);

  ChatroomInfo chatroom2;
  chatroom2.setName(ndn::Name::Component("supper-talk"));
  chatroom2.addParticipant(Name("/ndn/ucla/bob"));
  chatroom2.addParticipant(Name("/ndn/ucla/peter"));
  chatroom2.setTrustModel(ChatroomInfo::TRUST_MODEL_HIERARCHICAL);
  chatroomDiscoveryLogic.addLocalChatroom(chatroom2);

  //discovery
  chatroomDiscoveryLogic.onDiscoveryInterest(ChatroomDiscoveryLogic::DISCOVERY_PREFIX, discovery);

  face->processEvents(time::milliseconds(100));

  BOOST_CHECK_EQUAL(face->m_sentInterests.size(), 1);//the interest is for nfd register
  BOOST_CHECK_EQUAL(face->m_sentDatas.size(), 1);

  ChatroomInfo chatroom;
  chatroom.wireDecode(face->m_sentDatas[0].getContent().blockFromValue());
  chatroom.setName(face->m_sentDatas[0].getName()
                                      .get(ChatroomDiscoveryLogic::OFFSET_CHATROOM_NAME));

  BOOST_CHECK_EQUAL(chatroom.getName(), chatroom2.getName());
  BOOST_CHECK_EQUAL(chatroom.getParticipants().size(),
                    chatroom2.getParticipants().size());
  BOOST_CHECK_EQUAL(chatroom.getParticipants()[0].toUri(),
                    chatroom2.getParticipants()[0].toUri());
  BOOST_CHECK_EQUAL(chatroom.getParticipants()[1].toUri(),
                    chatroom2.getParticipants()[1].toUri());
  BOOST_CHECK_EQUAL(chatroom.getTrustModel(),
                    chatroom2.getTrustModel());

  //refreshing
  face->m_sentInterests.clear();
  face->m_sentDatas.clear();

  Name name = ChatroomDiscoveryLogic::DISCOVERY_PREFIX;
  name.append(Name("supper-talk"));

  Interest refreshing(name);
  refreshing.setMustBeFresh(true);
  refreshing.setInterestLifetime(time::milliseconds(10000));

  chatroomDiscoveryLogic.onDiscoveryInterest(name, refreshing);
  face->processEvents(time::milliseconds(100));

  BOOST_CHECK_EQUAL(face->m_sentInterests.size(), 0);
  BOOST_CHECK_EQUAL(face->m_sentDatas.size(), 1);

  chatroom.wireDecode(face->m_sentDatas[0].getContent().blockFromValue());
  chatroom.setName(face->m_sentDatas[0].getName()
                                      .get(ChatroomDiscoveryLogic::OFFSET_CHATROOM_NAME));

  BOOST_CHECK_EQUAL(chatroom.getName(), chatroom2.getName());
  BOOST_CHECK_EQUAL(chatroom.getParticipants().size(),
                    chatroom2.getParticipants().size());
  BOOST_CHECK_EQUAL(chatroom.getParticipants()[0].toUri(),
                    chatroom2.getParticipants()[0].toUri());
  BOOST_CHECK_EQUAL(chatroom.getParticipants()[1].toUri(),
                    chatroom2.getParticipants()[1].toUri());
  BOOST_CHECK_EQUAL(chatroom.getTrustModel(),
                    chatroom2.getTrustModel());
}

BOOST_FIXTURE_TEST_CASE(refreshChatroomFunction, Fixture)
{
  ChatroomDiscoveryLogic chatroomDiscoveryLogic(face, bind(&Fixture::update, this, _1, _2));

  ChatroomInfo chatroom1;
  chatroom1.setName(ndn::Name::Component("lunch-talk"));
  chatroom1.addParticipant(Name("/ndn/ucla/ymj"));
  chatroom1.addParticipant(Name("/ndn/ucla/alice"));
  chatroom1.setTrustModel(ChatroomInfo::TRUST_MODEL_WEBOFTRUST);
  chatroomDiscoveryLogic.addLocalChatroom(chatroom1);

  chatroomDiscoveryLogic.refreshChatroom(ndn::Name::Component("lunch-talk"));
  face->processEvents(time::milliseconds(100));

  BOOST_CHECK_EQUAL(face->m_sentInterests.size(), 2);
  BOOST_CHECK_EQUAL(face->m_sentDatas.size(), 0);

  Name name = ChatroomDiscoveryLogic::DISCOVERY_PREFIX;
  name.append(Name("lunch-talk"));

  BOOST_CHECK_EQUAL(face->m_sentInterests[1].getName().toUri(), name.toUri());
  BOOST_CHECK_EQUAL(face->m_sentInterests[1].getExclude().size(), 0);
}

BOOST_FIXTURE_TEST_CASE(onReceiveData, Fixture)
{
  ChatroomDiscoveryLogic chatroomDiscoveryLogic(face, bind(&Fixture::update, this, _1, _2));

  //discovery
  Interest discovery(ChatroomDiscoveryLogic::DISCOVERY_PREFIX);
  discovery.setMustBeFresh(true);
  discovery.setInterestLifetime(time::milliseconds(10000));

  Name dataName(ChatroomDiscoveryLogic::DISCOVERY_PREFIX);
  dataName.append(ndn::Name::Component("lunch-talk"));

  ChatroomInfo chatroom1;
  chatroom1.setName(ndn::Name::Component("lunch-talk"));
  chatroom1.addParticipant(Name("/ndn/ucla/ymj"));
  chatroom1.addParticipant(Name("/ndn/ucla/alice"));
  chatroom1.setTrustModel(ChatroomInfo::TRUST_MODEL_WEBOFTRUST);

  shared_ptr<Data> chatroomInfo = make_shared<Data>(dataName);
  chatroomInfo->setFreshnessPeriod(time::seconds(10));
  chatroomInfo->setContent(chatroom1.wireEncode());

  chatroomDiscoveryLogic.onReceiveData(discovery, *chatroomInfo, false);

  face->processEvents(time::milliseconds(1000));

  ndn::Name::Component chatroomName("lunch-talk");

  BOOST_CHECK_EQUAL(chatroomDiscoveryLogic.getChatrooms().size(), 1);
  BOOST_CHECK_EQUAL(chatroom1.getName(),
                    chatroomDiscoveryLogic.getChatrooms().find(chatroomName)
                    ->second.getName());
  BOOST_CHECK_EQUAL(chatroom1.getParticipants().size(),
                    chatroomDiscoveryLogic
                    .getChatrooms().find(chatroomName)->second.getParticipants().size());
  BOOST_CHECK_EQUAL(chatroom1.getParticipants()[0].toUri(),
                    chatroomDiscoveryLogic
                    .getChatrooms().find(chatroomName)->second
                    .getParticipants()[0].toUri());
  BOOST_CHECK_EQUAL(chatroom1.getParticipants()[1].toUri(),
                    chatroomDiscoveryLogic
                    .getChatrooms().find(chatroomName)->second
                    .getParticipants()[1].toUri());
  BOOST_CHECK_EQUAL(chatroom1.getTrustModel(),
                    chatroomDiscoveryLogic
                    .getChatrooms().find(chatroomName)->second.getTrustModel());

  //refreshing
  Name name = ChatroomDiscoveryLogic::DISCOVERY_PREFIX;
  name.append(Name("lunch-talk"));

  Interest refreshing(name);
  refreshing.setMustBeFresh(true);
  refreshing.setInterestLifetime(time::milliseconds(10000));

  chatroomDiscoveryLogic.onReceiveData(discovery, *chatroomInfo, false);

  face->processEvents(time::milliseconds(1000));

  BOOST_CHECK_EQUAL(chatroomDiscoveryLogic.getChatrooms().size(), 1);
  BOOST_CHECK_EQUAL(chatroom1.getName(),
                    chatroomDiscoveryLogic.getChatrooms().find(chatroomName)
                    ->second.getName());
  BOOST_CHECK_EQUAL(chatroom1.getParticipants().size(),
                    chatroomDiscoveryLogic
                    .getChatrooms().find(chatroomName)->second.getParticipants().size());
  BOOST_CHECK_EQUAL(chatroom1.getParticipants()[0].toUri(),
                    chatroomDiscoveryLogic
                    .getChatrooms().find(chatroomName)->second
                    .getParticipants()[0].toUri());
  BOOST_CHECK_EQUAL(chatroom1.getParticipants()[1].toUri(),
                    chatroomDiscoveryLogic
                    .getChatrooms().find(chatroomName)->second
                    .getParticipants()[1].toUri());
  BOOST_CHECK_EQUAL(chatroom1.getTrustModel(),
                    chatroomDiscoveryLogic
                    .getChatrooms().find(chatroomName)->second.getTrustModel());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace test

} // namespace chronos
