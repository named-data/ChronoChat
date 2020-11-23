/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013 Regents of the University of California.
 * @author: Yingdi Yu <yingdi@cs.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#pragma clang diagnostic ignored "-Wunused-function"
#elif __GNUC__
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/encoding/buffer-stream.hpp>
#include <ndn-cxx/util/time.hpp>
#include <ndn-cxx/util/io.hpp>
#include "cryptopp.hpp"
#include "endorse-certificate.hpp"

namespace chronochat {
namespace tests {

using std::vector;
using std::string;

using ndn::KeyChain;
using ndn::security::Certificate;

BOOST_AUTO_TEST_SUITE(TestEndorseCertificate)

std::string
getTestFile(std::string path) {
  std::ifstream t(path);
  std::stringstream buffer;
  buffer << t.rdbuf();
  return buffer.str();
}

BOOST_AUTO_TEST_CASE(IdCert)
{
  const std::string testIdCert = getTestFile("test/cert/testid.cert");
  boost::iostreams::stream<boost::iostreams::array_source> is(testIdCert.c_str(),
                                                              testIdCert.size());
  std::shared_ptr<Certificate> idCert = ndn::io::load<Certificate>(is);

  BOOST_CHECK(static_cast<bool>(idCert));

  BOOST_CHECK_EQUAL(idCert->getName().toUri(),
    "/EndorseCertificateTests/KEY/6%C7%E3%06%EC%8CB%3D/self/%FD%00%00%01u%D55a%B3");
}

BOOST_AUTO_TEST_CASE(ConstructFromIdCert)
{
  const std::string testIdCert = getTestFile("test/cert/testid.cert");
  boost::iostreams::stream<boost::iostreams::array_source> is(testIdCert.c_str(),
                                                              testIdCert.size());
  std::shared_ptr<Certificate> idCert = ndn::io::load<Certificate>(is);

  Profile profile(*idCert);
  vector<string> endorseList;
  endorseList.push_back("email");
  endorseList.push_back("homepage");
  EndorseCertificate endorseCertificate(*idCert, profile, endorseList);

  KeyChain keyChain("pib-memory:", "tpm-memory:");

  auto signOpts = ndn::security::SigningInfo(ndn::security::SigningInfo::SignerType::SIGNER_TYPE_SHA256);
  keyChain.sign(endorseCertificate, signOpts.setSignatureInfo(endorseCertificate.getSignatureInfo()));
  const Block& endorseDataBlock = endorseCertificate.wireEncode();

  Data decodedEndorseData;
  decodedEndorseData.wireDecode(endorseDataBlock);
  EndorseCertificate decodedEndorse(decodedEndorseData);
  BOOST_CHECK_EQUAL(decodedEndorse.getProfile().get("IDENTITY"),
                    "/EndorseCertificateTests");
  BOOST_CHECK_EQUAL(decodedEndorse.getEndorseList().size(), 2);
  BOOST_CHECK_EQUAL(decodedEndorse.getEndorseList().at(0), "email");
  BOOST_CHECK_EQUAL(decodedEndorse.getEndorseList().at(1), "homepage");
  BOOST_CHECK_EQUAL(decodedEndorse.getSigner(),
                    "/EndorseCertificateTests/KEY/6%C7%E3%06%EC%8CB%3D");
  BOOST_CHECK_EQUAL(decodedEndorse.getKeyName(),
                    "/EndorseCertificateTests/PROFILE-CERT/KEY/6%C7%E3%06%EC%8CB%3D");

  const std::string testIdKey = getTestFile("test/cert/testid.key");
  ndn::OBufferStream keyOs;
  {
    using namespace CryptoPP;
    StringSource(testIdKey, true, new Base64Decoder(new FileSink(keyOs)));
  }
  BOOST_CHECK(idCert->getPublicKey() == *keyOs.buf());
}

BOOST_AUTO_TEST_CASE(ConstructFromEndorseCert)
{
  const std::string testEndorseCert = getTestFile("test/cert/endorse.cert");
  boost::iostreams::stream<boost::iostreams::array_source> is(testEndorseCert.c_str(),
                                                              testEndorseCert.size());
  shared_ptr<Data> rawData = ndn::io::load<Data>(is);

  EndorseCertificate rawEndorse(*rawData);
  vector<string> endorseList;
  endorseList.push_back("institution");
  endorseList.push_back("group");
  endorseList.push_back("advisor");
  Name signer("/EndorseCertificateTests/Singer/ksk-1234567890");
  EndorseCertificate endorseCertificate(rawEndorse, signer, endorseList);

  KeyChain keyChain("pib-memory:", "tpm-memory:");

  auto signOpts = ndn::security::SigningInfo(ndn::security::SigningInfo::SignerType::SIGNER_TYPE_SHA256);
  keyChain.sign(endorseCertificate, signOpts.setSignatureInfo(endorseCertificate.getSignatureInfo()));

  const Block& endorseDataBlock = endorseCertificate.wireEncode();

  Data decodedEndorseData;
  decodedEndorseData.wireDecode(endorseDataBlock);
  EndorseCertificate decodedEndorse(decodedEndorseData);
  BOOST_CHECK_EQUAL(decodedEndorse.getProfile().get("IDENTITY"),
                    "/EndorseCertificateTests");
  BOOST_CHECK_EQUAL(decodedEndorse.getEndorseList().size(), 3);
  BOOST_CHECK_EQUAL(decodedEndorse.getEndorseList().at(0), "institution");
  BOOST_CHECK_EQUAL(decodedEndorse.getEndorseList().at(1), "group");
  BOOST_CHECK_EQUAL(decodedEndorse.getEndorseList().at(2), "advisor");
  BOOST_CHECK_EQUAL(decodedEndorse.getSigner(),
                    "/EndorseCertificateTests/Singer/ksk-1234567890");
  BOOST_CHECK_EQUAL(decodedEndorse.getKeyName(),
                    "/EndorseCertificateTests/KEY/6%C7%E3%06%EC%8CB%3D");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace chronochat
