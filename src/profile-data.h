/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef PROFILE_DATA_H
#define PROFILE_DATA_H

#include <ndn.cxx/data.h>

class ProfileData : public ndn::Data
{
public:
  ProfileData (const ndn::Name& identityName,
               const std::string& profileType,
               const ndn::Blob& profileValue);

  ProfileData (const ProfileData& profile);

  ProfileData (const ndn::Data& data);

  ~ProfileData () {}

  inline const ndn::Name&
  getIdentityName() const
  { return m_identityName; }

  inline const std::string&
  getProfileType() const
  { return m_profileType; }

private:
  ndn::Name m_identityName;
  std::string m_profileType;
};

#endif
