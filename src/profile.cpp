/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
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

void
Profile::encode(std::ostream& os) const
{
  chronochat::ProfileMsg profileMsg;
  profileMsg << (*this);
  profileMsg.SerializeToOstream(&os);
}

void
Profile::decode(std::istream& is)
{
  chronochat::ProfileMsg profileMsg;
  profileMsg.ParseFromIstream(&is);
  profileMsg >> (*this);
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

chronochat::ProfileMsg&
operator<<(chronochat::ProfileMsg& profileMsg, const Profile& profile)
{
;
  for (map<string, string>::const_iterator it = profile.begin(); it != profile.end(); it++) {
    chronochat::ProfileMsg::ProfileEntry* profileEntry = profileMsg.add_entry();
    profileEntry->set_oid(it->first);
    profileEntry->set_data(it->second);
  }
  return profileMsg;
}

chronochat::ProfileMsg&
operator>>(chronochat::ProfileMsg& profileMsg, Profile& profile)
{
  for (int i = 0; i < profileMsg.entry_size(); i++) {
    const chronochat::ProfileMsg::ProfileEntry& profileEntry = profileMsg.entry(i);
    profile[profileEntry.oid()] = profileEntry.data();
  }

  return profileMsg;
}

} // namespace chronochat
