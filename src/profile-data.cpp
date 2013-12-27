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
#include <boost/date_time/posix_time/posix_time.hpp>
#include "exception.h"
#include "logging.h"


using namespace ndn;
using namespace std;
using namespace boost::posix_time;

INIT_LOGGER("ProfileData");

ProfileData::ProfileData()
  : Data()
{}

ProfileData::ProfileData(const Profile& profile)
  : Data()
  , m_identity(profile.getIdentityName())
  , m_profile(profile)
{
  Name dataName = m_identity;
  
  time_duration now = microsec_clock::universal_time () - ptime(boost::gregorian::date (1970, boost::gregorian::Jan, 1));
  uint64_t version = (now.total_seconds () << 12) | (0xFFF & (now.fractional_seconds () / 244));
  dataName.append("PROFILE").appendVersion(version);
  setName(dataName);

  string content;
  profile.encode(&content);
  setContent((const uint8_t *)&content[0], content.size());

  getMetaInfo().setTimestampMilliseconds(time(NULL) * 1000.0);

}

// ProfileData::ProfileData(const ProfileData& profileData)
//   : Data(profileData)
//   , m_identity(profileData.m_identity)
//   , m_profile(profileData.m_profile)
// {}

ProfileData::ProfileData(const Data& data)
  : Data(data)
{
  const Name& dataName = data.getName();
  Name::Component appFlag(Name::fromEscapedString("PROFILE"));  

  int profileIndex = -1;
  for(int i = 0; i < dataName.size(); i++)
    {
      if(0 == dataName.get(i).compare(appFlag))
	{
	  profileIndex = i;
	  break;
	}
    }

  if(profileIndex < 0)
    throw LnException("No PROFILE component in data name!");

  m_identity = dataName.getPrefix(profileIndex);

  string encoded((const char*)data.getContent().buf(), data.getContent().size());
  m_profile = *Profile::decode(encoded);
}
