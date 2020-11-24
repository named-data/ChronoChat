/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2020, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#include "endorse-collection.hpp"

#include <boost/test/unit_test.hpp>
#include <ndn-cxx/encoding/buffer-stream.hpp>

namespace chronochat {
namespace tests {

using std::string;

BOOST_AUTO_TEST_SUITE(TestEndorseCollection)

BOOST_AUTO_TEST_CASE(EncodeDecode)
{
  EndorseCollection collection;
  collection.addCollectionEntry(ndn::Name("/ndn/ucla/qiuhan"), "hash");
  Block collectionWire;
  BOOST_REQUIRE_NO_THROW(collectionWire = collection.wireEncode());
  EndorseCollection decodedCollection;
  BOOST_REQUIRE_NO_THROW(decodedCollection.wireDecode(collectionWire));

  BOOST_CHECK_EQUAL(decodedCollection.getCollectionEntries().size(), 1);
  BOOST_CHECK_EQUAL(decodedCollection.getCollectionEntries()[0].certName.toUri(),
                    string("/ndn/ucla/qiuhan"));
  BOOST_CHECK_EQUAL(decodedCollection.getCollectionEntries()[0].hash, "hash");

}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace chronochat
