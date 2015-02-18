/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#include <boost/test/unit_test.hpp>

#include "conf.hpp"
#include <ndn-cxx/encoding/buffer-stream.hpp>

namespace chronochat{
namespace tests {

using std::string;

BOOST_AUTO_TEST_SUITE(TestConf)

BOOST_AUTO_TEST_CASE(EncodeDecode)
{
  string nick("qiuhan");
  Name identity("/ndn/edu/ucla/qiuhan");
  Conf conf;
  conf.setIdentity(identity);
  conf.setNick(nick);

  Block confWire;
  BOOST_REQUIRE_NO_THROW(confWire = conf.wireEncode());
  Conf decodedConf;
  BOOST_REQUIRE_NO_THROW(decodedConf.wireDecode(confWire));

  BOOST_CHECK_EQUAL(decodedConf.getNick(), nick);
  BOOST_CHECK_EQUAL(decodedConf.getIdentity(), identity);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace chronochat
