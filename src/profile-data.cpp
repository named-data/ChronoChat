/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "profile-data.h"

using namespace std;
using namespace ndn;

ProfileData::ProfileData (const Name& identityName,
			  const string& profileType,
			  const Blob& profileValue)
  : Data()
  , m_identityName(identityName)
  , m_profileType(profileType)
{
  Name tmpName = identityName;
  setName(tmpName.append(profileType));
  setContent(Content(profileValue.buf(), profileValue.size()));
}


ProfileData::ProfileData (const ProfileData& profile)
  : Data()
  , m_identityName(profile.m_identityName)
  , m_profileType(profile.m_profileType)
{
  setName(profile.getName());
  setContent(profile.getContent());
}

Ptr<ProfileData> 
ProfileData::fromData (const Data& data)
{
  //TODO:
  return NULL;
}

