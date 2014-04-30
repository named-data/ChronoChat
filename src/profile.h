/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOS_PROFILE_H
#define CHRONOS_PROFILE_H

#include "config.h"
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/security/identity-certificate.hpp>
#include <map>
#include <string>
#include "profile.pb.h"

namespace chronos{

class Profile
{
public:
  typedef std::map<std::string, std::string>::iterator iterator;
  typedef std::map<std::string, std::string>::const_iterator const_iterator;

  Profile() {}

  Profile(const ndn::IdentityCertificate& identityCertificate);

  Profile(const ndn::Name& identityName);

  Profile(const ndn::Name& identityName,
          const std::string& name,
          const std::string& institution);

  Profile(const Profile& profile);

  ~Profile() {}

  std::string&
  operator [] (const std::string& profileKey)
  { return m_entries[profileKey]; }

  std::string
  get (const std::string& profileKey) const
  {
    std::map<std::string, std::string>::const_iterator it = m_entries.find(profileKey);
    if(it != m_entries.end())
      return it->second;
    else
      return std::string();
  }

  inline Profile::iterator
  begin()
  { return m_entries.begin(); }

  inline Profile::const_iterator
  begin() const
  { return m_entries.begin(); }

  inline Profile::iterator
  end()
  { return m_entries.end(); }

  inline Profile::const_iterator
  end() const
  { return m_entries.end(); }

  void
  encode(std::ostream& os) const;

  void
  decode(std::istream& is);

  ndn::Name
  getIdentityName() const
  { return ndn::Name(m_entries.at("IDENTITY")); }

  inline bool
  operator == (const Profile& profile) const;

  inline bool
  operator != (const Profile& profile) const;

private:
  static const std::string OID_NAME;
  static const std::string OID_ORG;
  static const std::string OID_GROUP;
  static const std::string OID_HOMEPAGE;
  static const std::string OID_ADVISOR;
  static const std::string OID_EMAIL;

  std::map<std::string, std::string> m_entries;
};

Chronos::ProfileMsg&
operator << (Chronos::ProfileMsg& msg, const Profile& profile);

Chronos::ProfileMsg&
operator >> (Chronos::ProfileMsg& msg, Profile& profile);

inline bool
Profile::operator == (const Profile& profile) const
{
  if(m_entries.size() != profile.m_entries.size())
    return false;

  std::map<std::string, std::string>::const_iterator it = m_entries.begin();
  for(; it != m_entries.end(); it++)
    {
      std::map<std::string, std::string>::const_iterator found = profile.m_entries.find(it->first);
      if(found == profile.m_entries.end())
        return false;
      if(found->second != it->second)
        return false;
    }

  return true;
}

inline bool
Profile::operator != (const Profile& profile) const
{
  return !(*this == profile);
}

}//chronos

#endif
