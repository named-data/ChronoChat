/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013 Regents of the University of California.
 * @author: Yingdi Yu <yingdi@cs.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#include <profile.h>

#define BOOST_TEST_MODULE ChronoChat
#include <boost/test/unit_test.hpp>

using namespace ndn;
using namespace std;

BOOST_AUTO_TEST_SUITE(ProfileTests)

BOOST_AUTO_TEST_CASE(WriteRead)
{
  Name identity("/ndn/ucla/yingdi");
  Profile profile(identity);
  profile.setProfileEntry(string("name"), string("Yingdi Yu"));
  profile.setProfileEntry(string("school"), string("UCLA"));

  string encoded;
  profile.encode(&encoded);

  ptr_lib::shared_ptr<Profile> decodedProfile = Profile::decode(encoded);
  
  BOOST_CHECK_EQUAL(decodedProfile->getIdentityName().toUri(), string("/ndn/ucla/yingdi"));
  BOOST_CHECK_EQUAL(decodedProfile->getProfileEntry(string("name")), string("Yingdi Yu"));
  BOOST_CHECK_EQUAL(decodedProfile->getProfileEntry(string("school")), string("UCLA"));
}

BOOST_AUTO_TEST_SUITE_END()
