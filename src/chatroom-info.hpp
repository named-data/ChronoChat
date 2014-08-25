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
#ifndef CHRONOCHAT_CHATROOM_INFO_HPP
#define CHRONOCHAT_CHATROOM_INFO_HPP

#include "common.hpp"
#include "chatroom-tlv.hpp"
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/name-component.hpp>
#include <ndn-cxx/util/time.hpp>
#include <ndn-cxx/util/concepts.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/encoding-buffer.hpp>
#include <ndn-cxx/exclude.hpp>
#include <boost/concept_check.hpp>

namespace chronos {

/** \brief store a chatroom's information with encode and decode method.
    \sa docs/design.rst
 */

class ChatroomInfo
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


  enum TrustModel {
    TRUST_MODEL_HIERARCHICAL = 2,
    TRUST_MODEL_WEBOFTRUST = 1,
    TRUST_MODEL_NONE = 0
  };

public:

  ChatroomInfo();

  explicit
  ChatroomInfo(const Block& chatroomWire);

  const Block&
  wireEncode() const;

  void
  wireDecode(const Block& chatroomWire);

  const Name::Component&
  getName() const;

  void
  setName(const Name::Component& name);

  const std::vector<Name>&
  getParticipants() const;

  void
  addParticipant(const Name& participant);

  const std::vector<Name>&
  getContacts() const;

  void
  addContact(const Name& contact);

  const TrustModel
  getTrustModel() const;

  void
  setTrustModel(const TrustModel trustModel);

private:
  template<bool T>
  size_t
  wireEncode(ndn::EncodingImpl<T>& block) const;

private:
  mutable Block m_wire;
  Name::Component m_name;
  std::vector<Name> m_participants;
  TrustModel m_trustModel;
  std::vector<Name> m_contacts;

};

inline const Name::Component&
ChatroomInfo::getName() const
{
  return m_name;
}

inline const std::vector<Name>&
ChatroomInfo::getParticipants() const
{
  return m_participants;
}

inline const std::vector<Name>&
ChatroomInfo::getContacts() const
{
  return m_contacts;
}

inline const ChatroomInfo::TrustModel
ChatroomInfo::getTrustModel() const
{
  return m_trustModel;
}

BOOST_CONCEPT_ASSERT((ndn::WireEncodable<ChatroomInfo>));
BOOST_CONCEPT_ASSERT((ndn::WireDecodable<ChatroomInfo>));


} // namespace chronos

#endif //CHRONOCHAT_CHATROOM_INFO_HPP
