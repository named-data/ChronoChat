/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef LINKNDN_PROFILE_DATA_H
#define LINKNDN_PROFILE_DATA_H

#include <ndn-cpp-dev/data.hpp>
#include "profile.h"

class ProfileData : public ndn::Data
{
public:
  struct Error : public ndn::Data::Error { Error(const std::string &what) : ndn::Data::Error(what) {} };

  ProfileData();

  ProfileData(const Profile& profile);

  // ProfileData(const ProfileData& profileData);

  ProfileData(const ndn::Data& data);

  ~ProfileData() {}

  const ndn::Name& 
  getIdentityName() const
  { return m_identity; }

  const Profile&
  getProfile() const
  { return m_profile; }

private:
  ndn::Name m_identity;
  Profile m_profile;
};

#endif
