/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013 Regents of the University of California.
 * @author: Yingdi Yu <yingdi@cs.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#include <boost/test/unit_test.hpp>

#include "profile-data.h"
#include <ndn-cpp-dev/security/key-chain.hpp>
#include <ndn-cpp-dev/util/time.hpp>

using namespace ndn;
using namespace std;
using namespace chronos;

BOOST_AUTO_TEST_SUITE(TestProfile)

BOOST_AUTO_TEST_CASE(EncodeDecodeProfile)
{
  Name identity("/ndn/ucla/yingdi");
  Profile profile(identity);
  profile["name"] = "Yingdi Yu";
  profile["school"] = "UCLA";

  OBufferStream os;
  profile.encode(os);

  shared_ptr<Buffer> encoded = os.buf();
  
  boost::iostreams::stream
    <boost::iostreams::array_source> is (reinterpret_cast<const char*>(encoded->buf ()), encoded->size ());

  Profile decodedProfile; 
  decodedProfile.decode(is);
  
  BOOST_CHECK_EQUAL(decodedProfile.getIdentityName().toUri(), string("/ndn/ucla/yingdi"));
  BOOST_CHECK_EQUAL(decodedProfile["name"], string("Yingdi Yu"));
  BOOST_CHECK_EQUAL(decodedProfile["school"], string("UCLA"));
}

BOOST_AUTO_TEST_CASE(EncodeDecodeProfileData)
{
  Name identity("/ndn/ucla/yingdi");
  Profile profile(identity);
  profile["name"] = "Yingdi Yu";
  profile["school"] = "UCLA";

  ProfileData profileData(profile);
  
  KeyChainImpl<SecPublicInfoSqlite3, SecTpmFile> keyChain;
  Name signingIdentity("/EncodeDecodeProfile/EncodeDecodeProfileData");
  keyChain.createIdentity(signingIdentity);
  keyChain.signByIdentity(profileData, signingIdentity);

  Block encodedBlock = profileData.wireEncode();

  Data decodedData;
  decodedData.wireDecode(encodedBlock);
  
  ProfileData decodedProfileData(decodedData);
  Profile decodedProfile = decodedProfileData.getProfile();

  BOOST_CHECK_EQUAL(profileData.getIdentityName(), string("/ndn/ucla/yingdi"));
  BOOST_CHECK_EQUAL(decodedProfile["name"], string("Yingdi Yu"));
  BOOST_CHECK_EQUAL(decodedProfile["school"], string("UCLA"));
  
  keyChain.deleteIdentity(signingIdentity);
}


BOOST_AUTO_TEST_SUITE_END()
