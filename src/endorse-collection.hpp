/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_ENDORSE_COLLECTION_HPP
#define CHRONOCHAT_ENDORSE_COLLECTION_HPP

#include "common.hpp"
#include "tlv.hpp"
#include <ndn-cxx/util/concepts.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/encoding-buffer.hpp>

namespace chronochat {

class EndorseCollection
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

  class CollectionEntry
  {
  public:
    Name certName;
    std::string hash;
  };

public:

  EndorseCollection();

  explicit
  EndorseCollection(const Block& endorseWire);

  const Block&
  wireEncode() const;

  void
  wireDecode(const Block& endorseWire);

  const std::vector<CollectionEntry>&
  getCollectionEntries() const;

  void
  addCollectionEntry(const Name& certName, const std::string& hash);

private:
  template<bool T>
  size_t
  wireEncode(ndn::EncodingImpl<T>& block) const;

private:
  mutable Block m_wire;
  std::vector<CollectionEntry> m_entries;
};

inline const std::vector<EndorseCollection::CollectionEntry>&
EndorseCollection::getCollectionEntries() const
{
  return m_entries;
}

} // namespace chronochat

#endif //CHRONOCHAT_ENDORSE_COLLECTION_HPP
