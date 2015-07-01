/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Mengjin Yan <jane.yan0129@gmail.com>
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */
#ifndef CHRONOCHAT_CHATROOM_INFO_HPP
#define CHRONOCHAT_CHATROOM_INFO_HPP

#include "common.hpp"
#include "tlv.hpp"
#include <ndn-cxx/name-component.hpp>
#include <ndn-cxx/util/concepts.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/encoding-buffer.hpp>
#include <boost/concept_check.hpp>
#include <list>

namespace chronochat {

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

  const TrustModel
  getTrustModel() const;

  const Name&
  getSyncPrefix() const;

  const Name&
  getManagerPrefix() const;

  const std::list<Name>&
  getParticipants() const;

  void
  setName(const Name::Component& name);

  void
  setTrustModel(const TrustModel trustModel);

  void
  addParticipant(const Name& participant);

  void
  removeParticipant(const Name& participant);

  void
  setSyncPrefix(const Name& prefix);

  void
  setManager(const Name& manager);

private:
  template<bool T>
  size_t
  wireEncode(ndn::EncodingImpl<T>& encoder) const;

private:
  mutable Block m_wire;
  Name::Component m_chatroomName;
  std::list<Name> m_participants;
  Name m_manager;
  Name m_syncPrefix;
  TrustModel m_trustModel;

};

inline const Name::Component&
ChatroomInfo::getName() const
{
  return m_chatroomName;
}

inline const ChatroomInfo::TrustModel
ChatroomInfo::getTrustModel() const
{
  return m_trustModel;
}


inline const Name&
ChatroomInfo::getManagerPrefix() const
{
  return m_manager;
}

inline const Name&
ChatroomInfo::getSyncPrefix() const
{
  return m_syncPrefix;
}

inline const std::list<Name>&
ChatroomInfo::getParticipants() const
{
  return m_participants;
}

} // namespace chronochat

#endif // CHRONOCHAT_CHATROOM_INFO_HPP
