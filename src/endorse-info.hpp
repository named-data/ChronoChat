/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_ENDORSE_INFO_HPP
#define CHRONOCHAT_ENDORSE_INFO_HPP

#include "common.hpp"
#include "tlv.hpp"
#include <ndn-cxx/util/concepts.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/encoding-buffer.hpp>

namespace chronochat {

class EndorseInfo
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

  class Endorsement {
  public:
    std::string type;
    std::string value;
    std::string count;
  };

public:

  EndorseInfo();

  explicit
  EndorseInfo(const Block& endorseWire);

  const Block&
  wireEncode() const;

  void
  wireDecode(const Block& endorseWire);

  const std::vector<Endorsement>&
  getEndorsements() const;

  void
  addEndorsement(const std::string& type, const std::string& value, const std::string& count);

private:
  template<bool T>
  size_t
  wireEncode(ndn::EncodingImpl<T>& block) const;

private:
  mutable Block m_wire;
  std::vector<Endorsement> m_endorsements;
};

inline const std::vector<EndorseInfo::Endorsement>&
EndorseInfo::getEndorsements() const
{
  return m_endorsements;
}


} // namespace chronochat

#endif //CHRONOCHAT_ENDORSE_INFO_HPP
