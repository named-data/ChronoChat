/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#include "endorse-extension.hpp"

namespace chronochat {

BOOST_CONCEPT_ASSERT((ndn::WireEncodable<EndorseExtension>));
BOOST_CONCEPT_ASSERT((ndn::WireDecodable<EndorseExtension>));

EndorseExtension::EndorseExtension()
{
}

EndorseExtension::EndorseExtension(const Block& endorseWire)
{
  this->wireDecode(endorseWire);
}

template<bool T>
size_t
EndorseExtension::wireEncode(ndn::EncodingImpl<T>& block) const
{
  size_t totalLength = 0;

  // EndorseExtension := ENDORSE-EXTENSION-TYPE TLV-LENGTH
  //                       EntryData+
  //
  // EntryData := ENTRYDATA-TYPE TLV-LENGTH
  //                String
  //

  // EntryData
  for (std::list<std::string>::const_reverse_iterator it = m_entries.rbegin();
       it != m_entries.rend(); it++) {
    const uint8_t *entryWire = reinterpret_cast<const uint8_t*>(it->c_str());
    totalLength += block.prependByteArrayBlock(tlv::EntryData, entryWire, it->length());
  }

  // EndorseExtension
  totalLength += block.prependVarNumber(totalLength);
  totalLength += block.prependVarNumber(tlv::EndorseExtension);

  return totalLength;
}

const Block&
EndorseExtension::wireEncode() const
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
EndorseExtension::wireDecode(const Block& endorseWire)
{
  m_wire = endorseWire;
  m_wire.parse();

  // EndorseExtension := ENDORSE-EXTENSION-TYPE TLV-LENGTH
  //                       EntryData+
  //
  // EntryData := ENTRYDATA-TYPE TLV-LENGTH
  //                String
  //

  if (m_wire.type() != tlv::EndorseExtension)
    throw Error("Unexpected TLV number when decoding endorse extension packet");

  // EntryData
  Block::element_const_iterator i = m_wire.elements_begin();
  if (i == m_wire.elements_end())
    throw Error("Missing Entry Data");
  if (i->type() != tlv::EntryData)
    throw Error("Expect Entry Data but get TLV Type " + std::to_string(i->type()));

   while (i != m_wire.elements_end() && i->type() == tlv::EntryData) {
     m_entries.push_back(std::string(reinterpret_cast<const char* >(i->value()),
                                     i->value_size()));
     ++i;
  }

  if (i != m_wire.elements_end()) {
    throw Error("Unexpected element");
  }
}

void
EndorseExtension::addEntry(const std::string& entry)
{
  if (find(m_entries.begin(), m_entries.end(), entry) == m_entries.end()) {
    m_entries.push_back(entry);
  }
}

void
EndorseExtension::removeEntry(const std::string& entry)
{
  m_entries.remove(entry);
}

} // namespace chronochat
