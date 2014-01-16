/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef LINKNDN_PROFILE_H
#define LINKNDN_PROFILE_H

#include <ndn-cpp/name.hpp>
#include <ndn-cpp/security/identity-certificate.hpp>
#include <map>
#include <string>
#include "profile.pb.h"

class Profile
{
public:
  typedef std::map<std::string, std::string>::iterator iterator;
  typedef std::map<std::string, std::string>::const_iterator const_iterator;
public:
  Profile() {}

  Profile(const ndn::IdentityCertificate& identityCertificate);

  Profile(const ndn::Name& identityName);

  Profile(const ndn::Name& identityName,
          const std::string& name,
          const std::string& institution);
  
  Profile(const Profile& profile);
  
  virtual
  ~Profile() {}

  void
  setProfileEntry(const std::string& profileType,
                  const std::string& profileValue);
  
  std::string
  getProfileEntry(const std::string& profileType) const;

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
  encode(std::string* output) const;

  static ndn::ptr_lib::shared_ptr<Profile>
  decode(const std::string& input);

  const std::map<std::string, std::string>&
  getEntries() const
  { return m_entries; }

  const ndn::Name&
  getIdentityName() const
  { return m_identityName; }

protected:
  ndn::Name m_identityName;
  std::map<std::string, std::string> m_entries;
};

#endif
