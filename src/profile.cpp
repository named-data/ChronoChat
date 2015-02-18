/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 *         Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#include "profile.hpp"
#include "logging.h"

namespace chronochat {

using std::vector;
using std::string;
using std::map;

using ndn::IdentityCertificate;
using ndn::CertificateSubjectDescription;

const std::string Profile::OID_NAME("2.5.4.41");
const std::string Profile::OID_ORG("2.5.4.11");
const std::string Profile::OID_GROUP("2.5.4.1");
const std::string Profile::OID_HOMEPAGE("2.5.4.3");
const std::string Profile::OID_ADVISOR("2.5.4.80");
const std::string Profile::OID_EMAIL("1.2.840.113549.1.9.1");

Profile::Profile(const IdentityCertificate& identityCertificate)
{
  Name keyName = IdentityCertificate::certificateNameToPublicKeyName(identityCertificate.getName());

  m_entries[string("IDENTITY")] = keyName.getPrefix(-1).toUri();

  const vector<CertificateSubjectDescription>& subList =
    identityCertificate.getSubjectDescriptionList();

  for (vector<CertificateSubjectDescription>::const_iterator it = subList.begin();
       it != subList.end(); it++) {
    const string oidStr = it->getOidString();
    string valueStr = it->getValue();
    if (oidStr == OID_NAME)
      m_entries["name"] = valueStr;
    else if (oidStr == OID_ORG)
      m_entries["institution"] = valueStr;
    else if (oidStr == OID_GROUP)
      m_entries["group"] = valueStr;
    else if (oidStr == OID_HOMEPAGE)
      m_entries["homepage"] = valueStr;
    else if (oidStr == OID_ADVISOR)
      m_entries["advisor"] = valueStr;
    else if (oidStr == OID_EMAIL)
      m_entries["email"] = valueStr;
    else
      m_entries[oidStr] = valueStr;
  }
}

Profile::Profile(const Name& identityName)
{
  m_entries["IDENTITY"] = identityName.toUri();
}

Profile::Profile(const Name& identityName,
                 const string& name,
                 const string& institution)
{
  m_entries["IDENTITY"] = identityName.toUri();
  m_entries["name"] = name;
  m_entries["institution"] = institution;
}

Profile::Profile(const Profile& profile)
  : m_entries(profile.m_entries)
{
}

template<bool T>
size_t
Profile::wireEncode(ndn::EncodingImpl<T>& block) const
{
  size_t totalLength = 0;

  // Profile := PROFILE-TYPE TLV-LENGTH
  //             ProfileEntry+
  //
  // ProfileEntry := PROFILEENTRY-TYPE TLV-LENGTH
  //                   Oid
  //                   EntryData
  //
  // Oid := OID-TYPE TLV-LENGTH
  //            String
  //
  // EntryData := ENTRYDATA-TYPE TLV-LENGTH
  //                  String

  // Entries
  size_t entryLength = 0;
  for (map<string, string>::const_reverse_iterator it = m_entries.rbegin();
       it != m_entries.rend(); it++) {
    // Entry Data
    const uint8_t* dataWire = reinterpret_cast<const uint8_t*>(it->second.c_str());
    entryLength += block.prependByteArrayBlock(tlv::EntryData, dataWire, it->second.length());
    // Oid
    const uint8_t* oidWire = reinterpret_cast<const uint8_t*>(it->first.c_str());
    entryLength += block.prependByteArrayBlock(tlv::Oid, oidWire, it->first.length());
    entryLength += block.prependVarNumber(entryLength);
    entryLength += block.prependVarNumber(tlv::ProfileEntry);
    totalLength += entryLength;
    entryLength = 0;
  }

  // Profile
  totalLength += block.prependVarNumber(totalLength);
  totalLength += block.prependVarNumber(tlv::Profile);

  return totalLength;
}



const Block&
Profile::wireEncode() const
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
Profile::wireDecode(const Block& profileWire)
{
  m_wire = profileWire;
  m_wire.parse();

  if (m_wire.type() != tlv::Profile)
    throw Error("Unexpected TLV number when decoding profile packet");

  Block::element_const_iterator i = m_wire.elements_begin();
  if (i == m_wire.elements_end())
    throw Error("Missing Profile Entry");
  if (i->type() != tlv::ProfileEntry)
    throw Error("Expect Profile Entry but get TLV Type " + std::to_string(i->type()));

  while (i != m_wire.elements_end() && i->type() == tlv::ProfileEntry) {
    Block temp = *i;
    temp.parse();
    Block::element_const_iterator j = temp.elements_begin();
    if (j == temp.elements_end())
      throw Error("Missing Oid");
    if (j->type() != tlv::Oid)
      throw Error("Expect Oid but get TLV Type" + std::to_string(j->type()));

    string Oid = std::string(reinterpret_cast<const char* >(j->value()),
                             j->value_size());
    ++j;
    if (j == temp.elements_end())
      throw Error("Missing EntryData");
    if (j->type() != tlv::EntryData)
      throw Error("Expect EntryData but get TLV Type " + std::to_string(j->type()));

    string EntryData = std::string(reinterpret_cast<const char* >(j->value()),
                                   j->value_size());
    ++j;
    if (j != temp.elements_end()) {
      throw Error("Unexpected element");
    }
    m_entries[Oid] = EntryData;
    ++i;
  }

  if (i != m_wire.elements_end()) {
      throw Error("Unexpected element");
  }

}

bool
Profile::operator==(const Profile& profile) const
{
  if (m_entries.size() != profile.m_entries.size())
    return false;

  for(map<string, string>::const_iterator it = m_entries.begin(); it != m_entries.end(); it++) {
    map<string, string>::const_iterator found = profile.m_entries.find(it->first);
    if (found == profile.m_entries.end())
      return false;
    if (found->second != it->second)
      return false;
  }

  return true;
}

bool
Profile::operator!=(const Profile& profile) const
{
  return !(*this == profile);
}

} // namespace chronochat
