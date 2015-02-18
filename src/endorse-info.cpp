/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#include "endorse-info.hpp"

namespace chronochat {

BOOST_CONCEPT_ASSERT((ndn::WireEncodable<EndorseInfo>));
BOOST_CONCEPT_ASSERT((ndn::WireDecodable<EndorseInfo>));


EndorseInfo::EndorseInfo()
{
}

EndorseInfo::EndorseInfo(const Block& endorseWire)
{
  this->wireDecode(endorseWire);
}

template<bool T>
size_t
EndorseInfo::wireEncode(ndn::EncodingImpl<T>& block) const
{
  size_t totalLength = 0;

  // EndorseInfo := ENDORSE-INFO-TYPE TLV-LENGTH
  //                  ENDORSEMENT+
  //
  // Endorsement := ENDORSEMENT-TYPE TLV-LENGTH
  //                  EndorseType
  //                  EndorseValue
  //                  EndorseCount
  //
  // EndorseType := ENDORSETYPE-TYPE TLV-LENGTH
  //                  String
  //
  // EndorseValue := ENDORSEVALUE-TYPE TLV-LENGTH
  //                   String
  // EndorseCount := ENDORSECOUNT-TYPE TLV-LENGTH
  //                   String

  // Entries
  size_t entryLength = 0;
  for (std::vector<Endorsement>::const_reverse_iterator it = m_endorsements.rbegin();
       it != m_endorsements.rend(); it++) {

    // Endorse Count
    const uint8_t *countWire = reinterpret_cast<const uint8_t*>(it->count.c_str());
    entryLength += block.prependByteArrayBlock(tlv::EndorseCount, countWire, it->count.length());

    // Endorse Value
    const uint8_t *valueWire = reinterpret_cast<const uint8_t*>(it->value.c_str());
    entryLength += block.prependByteArrayBlock(tlv::EndorseValue, valueWire, it->value.length());

    // Endorse Type
    const uint8_t *typeWire = reinterpret_cast<const uint8_t*>(it->type.c_str());
    entryLength += block.prependByteArrayBlock(tlv::EndorseType, typeWire, it->type.length());

    // Endorsement
    entryLength += block.prependVarNumber(entryLength);
    entryLength += block.prependVarNumber(tlv::Endorsement);
    totalLength += entryLength;
    entryLength = 0;
  }

  // Profile
  totalLength += block.prependVarNumber(totalLength);
  totalLength += block.prependVarNumber(tlv::EndorseInfo);

  return totalLength;

}

const Block&
EndorseInfo::wireEncode() const
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
EndorseInfo::wireDecode(const Block& endorseWire)
{
  m_wire = endorseWire;
  m_wire.parse();
  m_endorsements.clear();

  if (m_wire.type() != tlv::EndorseInfo)
    throw Error("Unexpected TLV number when decoding endorse info packet");

  // Endorsement
  Block::element_const_iterator i = m_wire.elements_begin();
  if (i == m_wire.elements_end())
    throw Error("Missing Endorsement");
  if (i->type() != tlv::Endorsement)
    throw Error("Expect Endorsement but get TLV Type " + std::to_string(i->type()));

  while (i != m_wire.elements_end() && i->type() == tlv::Endorsement) {
    Endorsement endorsement;
    Block temp = *i;
    temp.parse();
    Block::element_const_iterator j = temp.elements_begin();

    // Endorse Type
    if (j == temp.elements_end())
      throw Error("Missing Endorse Type");
    if (j->type() != tlv::EndorseType)
      throw Error("Expect Endorse Type but get TLV Type " + std::to_string(j->type()));

    endorsement.type = std::string(reinterpret_cast<const char* >(j->value()),
                                   j->value_size());;
    ++j;

    // Endorse Value
    if (j == temp.elements_end())
      throw Error("Missing Endorse Value");
    if (j->type() != tlv::EndorseValue)
      throw Error("Expect Endorse Value but get TLV Type " + std::to_string(j->type()));

    endorsement.value = std::string(reinterpret_cast<const char* >(j->value()),
                                    j->value_size());
    ++j;

    // Endorse Count
    if (j == temp.elements_end())
      throw Error("Missing Endorse Count");
    if (j->type() != tlv::EndorseCount)
      throw Error("Expect Endorse Count but get TLV Type " + std::to_string(j->type()));

    endorsement.count = std::string(reinterpret_cast<const char* >(j->value()),
                                    j->value_size());
    ++j;
    if (j != temp.elements_end()) {
      throw Error("Unexpected element");
    }
    m_endorsements.push_back(endorsement);
    ++i;
  }

  if (i != m_wire.elements_end()) {
      throw Error("Unexpected element");
  }
}

void
EndorseInfo::addEndorsement(const std::string& type, const std::string& value,
                            const std::string& count) {
  Endorsement endorsement;
  endorsement.type = type;
  endorsement.value = value;
  endorsement.count = count;
  m_endorsements.push_back(endorsement);
}

} // namespace chronochat
