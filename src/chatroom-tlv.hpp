#ifndef CHRONOCHAT_CHATROOM_TLV_HPP
#define CHRONOCHAT_CHATROOM_TLV_HPP

namespace chronochat {

namespace tlv {

enum {
  ChatroomInfo = 128,
  ChatroomName = 129,
  TrustModel = 130,
  ChatroomPrefix = 131,
  ManagerPrefix = 132,
  Participants = 133,
};

} // namespace tlv

} // namespace chronochat

#endif // CHRONOCHAT_CHATROOM_TLV_HPP
