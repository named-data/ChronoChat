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

#include "logging.h"

using namespace ndn;
using namespace std;

INIT_LOGGER("ProfileData");

namespace chronos{

ProfileData::ProfileData()
  : Data()
{}

ProfileData::ProfileData(const Profile& profile)
  : Data()
  , m_identity(profile.getIdentityName())
  , m_profile(profile)
{
  Name dataName = m_identity;
  dataName.append("PROFILE").appendVersion();
  setName(dataName);

  OBufferStream os;
  profile.encode(os);
  setContent(os.buf());
}

ProfileData::ProfileData(const Data& data)
  : Data(data)
{
  if(data.getName().get(-2).toEscapedString() == "PROFILE")
    throw Error("No PROFILE component in data name!");

  m_identity = data.getName().getPrefix(-2);

  boost::iostreams::stream <boost::iostreams::array_source> is 
    (reinterpret_cast<const char*>(data.getContent().value()), data.getContent().value_size());

  m_profile.decode(is);
}

}//chronos
