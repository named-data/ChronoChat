/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#include "endorse-collection.hpp"

namespace chronochat {

BOOST_CONCEPT_ASSERT((ndn::WireEncodable<EndorseCollection>));
BOOST_CONCEPT_ASSERT((ndn::WireDecodable<EndorseCollection>));

EndorseCollection::EndorseCollection()
{
}

EndorseCollection::EndorseCollection(const Block& endorseWire)
{
  this->wireDecode(endorseWire);
}

template<bool T>
size_t
EndorseCollection::wireEncode(ndn::EncodingImpl<T>& block) const
{
  size_t totalLength = 0;

  // EndorseCollection := ENDORSE-COLLECTION-TYPE TLV-LENGTH
  //                        EndorseCollectionEntry+
  //
  // EndorseCollectionEntry := ENDORSE-COLLECTION-ENTRY-TYPE TLV-LENGTH
  //                             Name
  //                             Hash
  //
  // Hash := HASH-TYPE TLV-LENGTH
  //           String

  // Entries
  size_t entryLength = 0;
  for (std::vector<CollectionEntry>::const_reverse_iterator it = m_entries.rbegin();
       it != m_entries.rend(); it++) {
    // Hash
    const uint8_t* dataWire = reinterpret_cast<const uint8_t*>(it->hash.c_str());
    entryLength += block.prependByteArrayBlock(tlv::Hash, dataWire, it->hash.length());
    // CertName
    entryLength += it->certName.wireEncode(block);
    // Entry
    entryLength += block.prependVarNumber(entryLength);
    entryLength += block.prependVarNumber(tlv::EndorseCollectionEntry);
    totalLength += entryLength;
    entryLength = 0;
  }

  // Profile
  totalLength += block.prependVarNumber(totalLength);
  totalLength += block.prependVarNumber(tlv::EndorseCollection);

  return totalLength;

}

const Block&
EndorseCollection::wireEncode() const
{
  ndn::EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator);

  ndn::EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer);

  m_wire = buffer.block();
  m_wire.parse();

  return m_wire;
}

void
EndorseCollection::wireDecode(const Block& endorseWire)
{
  m_wire = endorseWire;
  m_wire.parse();
  m_entries.clear();

  if (m_wire.type() != tlv::EndorseCollection)
    throw Error("Unexpected TLV number when decoding endorse collection packet");

  Block::element_const_iterator i = m_wire.elements_begin();
  if (i == m_wire.elements_end())
    throw Error("Missing Endorse Collection Entry");
  if (i->type() != tlv::EndorseCollectionEntry)
    throw Error("Expect Endorse Collection Entry but get TLV Type " + std::to_string(i->type()));

  while (i != m_wire.elements_end() && i->type() == tlv::EndorseCollectionEntry) {
    CollectionEntry entry;
    Block temp = *i;
    temp.parse();
    Block::element_const_iterator j = temp.elements_begin();
    if (j == temp.elements_end())
      throw Error("Missing Cert Name");
    if (j->type() != tlv::Name)
      throw Error("Expect Cert Name but get TLV Type " + std::to_string(j->type()));

    entry.certName.wireDecode(*j);

    ++j;
    if (j == temp.elements_end())
      throw Error("Missing Hash");
    if (j->type() != tlv::Hash)
      throw Error("Expect Hash but get TLV Type " + std::to_string(j->type()));

    entry.hash = std::string(reinterpret_cast<const char* >(j->value()),
                             j->value_size());
    ++j;
    if (j != temp.elements_end()) {
      throw Error("Unexpected element");
    }
    m_entries.push_back(entry);
    ++i;
  }

  if (i != m_wire.elements_end()) {
      throw Error("Unexpected element");
  }
}

void
EndorseCollection::addCollectionEntry(const Name& certName, const std::string& hash) {
  CollectionEntry entry;
  entry.certName = certName;
  entry.hash = hash;
  m_entries.push_back(entry);
}

} // namespace chronochat
