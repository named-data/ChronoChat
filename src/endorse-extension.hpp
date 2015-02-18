/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_ENDORSE_EXTENSION_HPP
#define CHRONOCHAT_ENDORSE_EXTENSION_HPP

#include "common.hpp"
#include "tlv.hpp"
#include <ndn-cxx/util/concepts.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/encoding-buffer.hpp>
#include <list>

namespace chronochat {

class EndorseExtension
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

public:

  EndorseExtension();

  explicit
  EndorseExtension(const Block& endorseWire);

  const Block&
  wireEncode() const;

  void
  wireDecode(const Block& endorseWire);

  const std::list<std::string>&
  getEntries() const;

  void
  addEntry(const std::string& entry);

  void
  removeEntry(const std::string& entry);

private:
  template<bool T>
  size_t
  wireEncode(ndn::EncodingImpl<T>& block) const;

private:
  mutable Block m_wire;
  std::list<std::string> m_entries;
};

inline const std::list<std::string>&
EndorseExtension::getEntries() const
{
  return m_entries;
}


} // namespace chronochat

#endif //CHRONOCHAT_ENDORSE_EXTENSION_HPP
