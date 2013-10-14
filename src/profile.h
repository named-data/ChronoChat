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

#include <ndn.cxx/common.h>
#include <ndn.cxx/fields/name.h>
#include <ndn.cxx/fields/blob.h>
#include <map>
#include <string>

class Profile
{
public:
  typedef std::map<std::string, ndn::Blob>::iterator iterator;
  typedef std::map<std::string, ndn::Blob>::const_iterator const_iterator;
public:
  Profile() {}

  Profile(const ndn::Name& identityName);

  Profile(const ndn::Name& identityName,
          const std::string& name,
          const std::string& institution);
  
  Profile(const Profile& profile);
  
  virtual
  ~Profile() {}

  void
  setProfileEntry(const std::string& profileType,
                  const ndn::Blob& profileValue);
  
  ndn::Ptr<const ndn::Blob>
  getProfileEntry(const std::string& profileType);

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

  ndn::Ptr<ndn::Blob>
  toDerBlob() const;

  static ndn::Ptr<Profile>
  fromDerBlob(const ndn::Blob& derBlob);

protected:
  ndn::Name m_identityName;
  std::map<std::string, ndn::Blob> m_entries;
};



#endif
