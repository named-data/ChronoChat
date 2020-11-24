/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2020 Regents of the University of California.
 * @author: Yingdi Yu <yingdi@cs.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#include "profile.hpp"

#include <boost/test/unit_test.hpp>
#include <ndn-cxx/encoding/buffer-stream.hpp>

namespace chronochat {
namespace tests {

using std::string;

BOOST_AUTO_TEST_SUITE(TestProfile)

BOOST_AUTO_TEST_CASE(EncodeDecodeProfile)
{
  Name identity("/ndn/ucla/yingdi");
  Profile profile(identity);
  profile["name"] = "Yingdi Yu";
  profile["school"] = "UCLA";

  Block profileWire = profile.wireEncode();

  Profile decodedProfile;
  decodedProfile.wireDecode(profileWire);

  BOOST_CHECK_EQUAL(decodedProfile.getIdentityName().toUri(), string("/ndn/ucla/yingdi"));
  BOOST_CHECK_EQUAL(decodedProfile["name"], string("Yingdi Yu"));
  BOOST_CHECK_EQUAL(decodedProfile["school"], string("UCLA"));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace chronochat
