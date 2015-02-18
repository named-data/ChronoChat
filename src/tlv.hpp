#ifndef CHRONOCHAT_TLV_HPP
#define CHRONOCHAT_TLV_HPP

namespace chronochat {

namespace tlv {

enum {
  ChatroomInfo = 128,
  ChatroomName = 129,
  TrustModel = 130,
  ChatroomPrefix = 131,
  ManagerPrefix = 132,
  Participants = 133,
  Conf = 134,
  Nick = 135,
  Profile = 136,
  ProfileEntry = 137,
  Oid = 138,
  EntryData = 139,
  EndorseExtension = 140,
  EndorseCollection = 141,
  EndorseCollectionEntry = 142,
  Hash = 143,
  EndorseInfo = 144,
  Endorsement = 145,
  EndorseType = 146,
  EndorseValue = 147,
  EndorseCount = 148,
  ChatMessage = 149,
  ChatMessageType = 150,
  ChatData = 151,
  Timestamp = 152,
};

} // namespace tlv

} // namespace chronochat

#endif // CHRONOCHAT_TLV_HPP
