/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#include "conf.hpp"

namespace chronochat {

BOOST_CONCEPT_ASSERT((ndn::WireEncodable<Conf>));
BOOST_CONCEPT_ASSERT((ndn::WireDecodable<Conf>));

Conf::Conf()
{
}

Conf::Conf(const Block& confWire)
{
  this->wireDecode(confWire);
}

template<bool T>
size_t
Conf::wireEncode(ndn::EncodingImpl<T>& block) const
{
  size_t totalLength = 0;

  // Conf := CONF-TYPE TLV-LENGTH
  //           Name
  //           Nick
  //
  // Nick := NICK-TYPE TLV-LENGTH
  //            String

  // Nick
  const uint8_t* nickWire = reinterpret_cast<const uint8_t*>(m_nick.c_str());
  totalLength += block.prependByteArrayBlock(tlv::Nick, nickWire, m_nick.length());

  // Identity
  totalLength += m_identity.wireEncode(block);

  // Conf
  totalLength += block.prependVarNumber(totalLength);
  totalLength += block.prependVarNumber(tlv::Conf);

  return totalLength;
}

const Block&
Conf::wireEncode() const
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
Conf::wireDecode(const Block& confWire)
{
  m_wire = confWire;
  m_wire.parse();

  // Conf := CONF-TYPE TLV-LENGTH
  //           Name
  //           Nick
  //
  // Nick := NICK-TYPE TLV-LENGTH
  //            String

  if (m_wire.type() != tlv::Conf)
    throw Error("Unexpected TLV number when decoding conf packet");

  // Identity
  Block::element_const_iterator i = m_wire.elements_begin();
  if (i == m_wire.elements_end())
    throw Error("Missing Identity");
  if (i->type() != tlv::Name)
    throw Error("Expect Identity but get TLV Type " + std::to_string(i->type()));

  m_identity.wireDecode(*i);
  ++i;

  // Nick
  if (i == m_wire.elements_end())
    return;
  if (i->type() != tlv::Nick)
    throw Error("Expect Nick but get TLV Type " + std::to_string(i->type()));

  m_nick = std::string(reinterpret_cast<const char* >(i->value()),
                       i->value_size());
  ++i;

  if (i != m_wire.elements_end()) {
    throw Error("Unexpected element");
  }
}

void
Conf::setIdentity(const Name& identity)
{
  m_wire.reset();
  m_identity = identity;
}

void
Conf::setNick(const std::string& nick)
{
  m_wire.reset();
  m_nick = nick;
}

} // namespace chronochat
