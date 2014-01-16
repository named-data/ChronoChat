/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "profile.h"
#include "logging.h"

using namespace std;
using namespace ndn;
using namespace ndn::ptr_lib;

INIT_LOGGER("Profile");

static string nameOid("2.5.4.41");
static string orgOid("2.5.4.11");
static string groupOid("2.5.4.1");
static string homepageOid("2.5.4.3");
static string advisor("2.5.4.80");
static string emailOid("1.2.840.113549.1.9.1");

Profile::Profile(const IdentityCertificate& oldIdentityCertificate)
{
  IdentityCertificate identityCertificate(oldIdentityCertificate);

  Name keyName = identityCertificate.getPublicKeyName();
  m_identityName = keyName.getPrefix(-1);

  const string& identityString = m_identityName.toUri();
  m_entries[string("IDENTITY")] = identityString;
  
  const vector<CertificateSubjectDescription>& subList = identityCertificate.getSubjectDescriptionList();
  vector<CertificateSubjectDescription>::const_iterator it = subList.begin();
  for(; it != subList.end(); it++)
    {
      const string oidStr = it->getOidString();
      string valueStr = it->getValue();
      if(oidStr == nameOid)
        m_entries[string("name")] = valueStr;
      else if(oidStr == orgOid)
        m_entries[string("institution")] = valueStr;
      else if(oidStr == groupOid)
        m_entries[string("group")] = valueStr;
      else if(oidStr == homepageOid)
        m_entries[string("homepage")] = valueStr;
      else if(oidStr == advisor)
        m_entries[string("advisor")] = valueStr;
      else if(oidStr == emailOid)
        m_entries[string("email")] = valueStr;
      else
        m_entries[oidStr] = valueStr;
    }
}

Profile::Profile(const Name& identityName)
  : m_identityName(identityName)
{
  const string& identityString = identityName.toUri();
  m_entries[string("IDENTITY")] = identityString;
}

Profile::Profile(const Name& identityName,
		 const string& name,
		 const string& institution)
  : m_identityName(identityName)
{
  const string& identityString = identityName.toUri();
  m_entries[string("IDENTITY")] = identityString;

  m_entries[string("name")] = name;
  m_entries[string("institution")] = institution;
}

Profile::Profile(const Profile& profile)
  : m_identityName(profile.m_identityName)
  , m_entries(profile.m_entries)
{}

void
Profile::setProfileEntry(const string& profileType,
			 const string& profileValue)
{ m_entries[profileType] = profileValue; }

string
Profile::getProfileEntry(const string& profileType) const
{
  if(m_entries.find(profileType) != m_entries.end())
    return m_entries.at(profileType);

  return string();
}

void
Profile::encode(string* output) const
{

  Chronos::ProfileMsg profileMsg;

  profileMsg.set_identityname(m_identityName.toUri());
  
  map<string, string>::const_iterator it = m_entries.begin();
  for(; it != m_entries.end(); it++)
    {
      Chronos::ProfileMsg::ProfileEntry* profileEntry = profileMsg.add_entry();
      profileEntry->set_oid(it->first);
      profileEntry->set_data(it->second);
    }

  profileMsg.SerializeToString(output);
}

shared_ptr<Profile>
Profile::decode(const string& input)
{
  Chronos::ProfileMsg profileMsg;
    
  profileMsg.ParseFromString(input);

  shared_ptr<Profile> profile = make_shared<Profile>(profileMsg.identityname());

  for(int i = 0; i < profileMsg.entry_size(); i++)
    {
      const Chronos::ProfileMsg::ProfileEntry& profileEntry = profileMsg.entry(i);
      profile->setProfileEntry(profileEntry.oid(), profileEntry.data());
    }

  return profile;
}
