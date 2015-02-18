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
using ndn::IdentityCertificate;

BOOST_AUTO_TEST_SUITE(TestEndorseCertificate)

const string testIdCert("\
Bv0DXwdRCBdFbmRvcnNlQ2VydGlmaWNhdGVUZXN0cwgDS0VZCAxFbmNvZGVEZWNv\
ZGUIEWtzay0xMzk0MDcyMTQ3MzM1CAdJRC1DRVJUCAf9AUSVLNXoFAMYAQIV/QG8\
MIIBuDAiGA8yMDE0MDMwNjAyMTU0N1oYDzIwMTQwMzEzMDkyNzQ3WjBuMA0GA1UE\
KRMGTXlOYW1lMBIGA1UECxMLTXlJbnN0aXR1dGUwDgYDVQQBEwdNeUdyb3VwMBEG\
A1UEAxMKTXlIb21lUGFnZTAQBgNVBFATCU15QWR2aXNvcjAUBgkqhkiG9w0BCQET\
B015RW1haWwwggEgMA0GCSqGSIb3DQEBAQUAA4IBDQAwggEIAoIBAQDYsWD0ixQF\
RfYs36BHNsRNv5ouEL69oaS6XX/hsQN1By4RNI6eSG5DpajtAwK1y+DXPwkLHd5S\
BrvwLzReF7SsrF2ObawznU14GKaQdbn+eVIER7CWvSpJhH5yKS4fCPRN+b1MP8QS\
DLvaaGu15T98cgVscIEqFkLfnWSQbdN6EnodjOH27JkBCz8Lxv9GZLrhfKGzOylR\
fLzvCIyIXYl6HWroO+xTJQaP+miSZNVGyf4jYqz5WbQH56a9ZjUldTphjuDbBjUq\
QaNVOzoKT+H4qh8mn399aQ9/BjM+6/WgrSw7/MO2UCgoZhySQY4HVqzUVVWnYwOU\
NYPoOS3HdvGLAgERFkEbAQEcPAc6CBdFbmRvcnNlQ2VydGlmaWNhdGVUZXN0cwgD\
S0VZCBFrc2stMTM5NDA3MjE0NzEyOAgHSUQtQ0VSVBf9AQARSwS/CelRRSUO4Tik\
5Q+L5zusaqq5652T92/83S5l38dO41BOf5fBUb3RtnFSbS/QaBCRfRJtDvkN2LhE\
vksJjSAoAKUzx27UyM1eq7L8DDvsvC9mbwxGzTK2F1t3Jy81rk5X34MecvztlILs\
nLqzqqiwl3dS1xyvg9GZez5g1yoOtRwzkHaah6svLVwzwM7kECXWRf4NoHTazWQo\
Cs6s60F9I/xBRKJ4Cw2L/AzvB5sX1J4HvHCsplbR/GdvA8uW6i8pp7kjIhjCGewK\
uNfH/4lHxzTl3pjsVy+EHKmwSlZ+T8cy5qaIEHxhbOzMNNVdit7XEwexOE66AVza\
92On");

const string testKey("\
MIIBIDANBgkqhkiG9w0BAQEFAAOCAQ0AMIIBCAKCAQEA2LFg9IsUBUX2LN+gRzbE\
Tb+aLhC+vaGkul1/4bEDdQcuETSOnkhuQ6Wo7QMCtcvg1z8JCx3eUga78C80Xhe0\
rKxdjm2sM51NeBimkHW5/nlSBEewlr0qSYR+cikuHwj0Tfm9TD/EEgy72mhrteU/\
fHIFbHCBKhZC351kkG3TehJ6HYzh9uyZAQs/C8b/RmS64XyhszspUXy87wiMiF2J\
eh1q6DvsUyUGj/pokmTVRsn+I2Ks+Vm0B+emvWY1JXU6YY7g2wY1KkGjVTs6Ck/h\
+KofJp9/fWkPfwYzPuv1oK0sO/zDtlAoKGYckkGOB1as1FVVp2MDlDWD6Dktx3bx\
iwIBEQ==");

const string testEndorseCert("\
Bv0CYweICBdFbmRvcnNlQ2VydGlmaWNhdGVUZXN0cwgMRW5jb2RlRGVjb2RlCBFr\
c2stMTM5NDA3MjE0NzMzNQgMUFJPRklMRS1DRVJUCDMHMQgXRW5kb3JzZUNlcnRp\
ZmljYXRlVGVzdHMIBlNpbmdlcggOa3NrLTEyMzQ1Njc4OTAICf0AAAFMoXR8NRQD\
GAECFf0BqTCCAaUwIhgPMjAxMzEyMjYyMzIyNTRaGA8yMDEzMTIyNjIzMjI1NFow\
QDA+BgNVBCkTNy9FbmRvcnNlQ2VydGlmaWNhdGVUZXN0cy9FbmNvZGVEZWNvZGUv\
a3NrLTEzOTQwNzIxNDczMzUwgZ0wDQYJKoZIhvcNAQEBBQADgYsAMIGHAoGBAJ4G\
PkeFsjQ3qoVHrAMkg7WcqAU6JB7riQG76ZuywyKsaOPwbALOaKbE0KcGkJyqGwgd\
i0OaM2dEbSGjG4ial15ZxBUL2Sy9UQdhgq3BuNe/m899JMJj85cX6/5iJbpbTYrC\
er1Dio+48vHFajDTUIzImt/v7TXnemLqdny7CCbHAgERMIGcMGsGBysGAQUgAgEB\
Af8EXYhbiTGKCElERU5USVRZiyUvRW5kb3JzZUNlcnRpZmljYXRlVGVzdHMvRW5j\
b2RlRGVjb2RliRaKCGhvbWVwYWdliwpNeUhvbWVQYWdliQ6KBG5hbWWLBk15TmFt\
ZTAtBgcrBgEFIAICAQH/BB+MHYsLaW5zdGl0dXRpb26LBWdyb3VwiwdhZHZpc29y\
FgMbAQAXIHalD2NUzM7abX6QY+2qWNLVMC+ch2xnVyrlf89ZH/IV");

BOOST_AUTO_TEST_CASE(IdCert)
{
  boost::iostreams::stream<boost::iostreams::array_source> is(testIdCert.c_str(),
                                                              testIdCert.size());
  shared_ptr<IdentityCertificate> idCert = ndn::io::load<IdentityCertificate>(is);

  BOOST_CHECK(static_cast<bool>(idCert));

  const ndn::Certificate::SubjectDescriptionList& subjectDescription =
    idCert->getSubjectDescriptionList();
  BOOST_CHECK_EQUAL(subjectDescription.size(), 6);

  ndn::Certificate::SubjectDescriptionList::const_iterator it  = subjectDescription.begin();
  ndn::Certificate::SubjectDescriptionList::const_iterator end = subjectDescription.end();
  int count = 0;
  for(; it != end; it++)
    {
      if(it->getOidString() == "2.5.4.41")
        {
          BOOST_CHECK_EQUAL(it->getValue(), "MyName");
          count++;
        }
      if(it->getOidString() == "2.5.4.11")
        {
          BOOST_CHECK_EQUAL(it->getValue(), "MyInstitute");
          count++;
        }
      if(it->getOidString() == "2.5.4.1")
        {
          BOOST_CHECK_EQUAL(it->getValue(), "MyGroup");
          count++;
        }
      if(it->getOidString() == "2.5.4.3")
        {
          BOOST_CHECK_EQUAL(it->getValue(), "MyHomePage");
          count++;
        }
      if(it->getOidString() == "2.5.4.80")
        {
          BOOST_CHECK_EQUAL(it->getValue(), "MyAdvisor");
          count++;
        }
      if(it->getOidString() == "1.2.840.113549.1.9.1")
        {
          BOOST_CHECK_EQUAL(it->getValue(), "MyEmail");
          count++;
        }
    }
  BOOST_CHECK_EQUAL(count, 6);

  BOOST_CHECK_EQUAL(idCert->getName().toUri(),
    "/EndorseCertificateTests/KEY/EncodeDecode/ksk-1394072147335/ID-CERT/%FD%01D%95%2C%D5%E8");

  ndn::OBufferStream keyOs;
  {
    using namespace CryptoPP;
    StringSource(testKey, true, new Base64Decoder(new FileSink(keyOs)));
  }
  ndn::PublicKey key(keyOs.buf()->buf(), keyOs.buf()->size());
  BOOST_CHECK(key == idCert->getPublicKeyInfo());
}

BOOST_AUTO_TEST_CASE(ConstructFromIdCert)
{
  boost::iostreams::stream<boost::iostreams::array_source> is(testIdCert.c_str(),
                                                              testIdCert.size());
  shared_ptr<IdentityCertificate> idCert = ndn::io::load<IdentityCertificate>(is);

  Profile profile(*idCert);
  vector<string> endorseList;
  endorseList.push_back("email");
  endorseList.push_back("homepage");
  EndorseCertificate endorseCertificate(*idCert, profile, endorseList);

  boost::filesystem::path keyChainTmpPath =
    boost::filesystem::path(TEST_CERT_PATH) / "TestEndorseCertificate";
  KeyChain keyChain(std::string("sqlite3:").append(keyChainTmpPath.string()),
                    std::string("tpm-file:").append(keyChainTmpPath.string()));

  keyChain.signWithSha256(endorseCertificate);
  const Block& endorseDataBlock = endorseCertificate.wireEncode();

  Data decodedEndorseData;
  decodedEndorseData.wireDecode(endorseDataBlock);
  EndorseCertificate decodedEndorse(decodedEndorseData);
  BOOST_CHECK_EQUAL(decodedEndorse.getProfile().get("IDENTITY"),
                    "/EndorseCertificateTests/EncodeDecode");
  BOOST_CHECK_EQUAL(decodedEndorse.getProfile().get("name"), "MyName");
  BOOST_CHECK_EQUAL(decodedEndorse.getProfile().get("homepage"), "MyHomePage");
  BOOST_CHECK_EQUAL(decodedEndorse.getEndorseList().size(), 2);
  BOOST_CHECK_EQUAL(decodedEndorse.getEndorseList().at(0), "email");
  BOOST_CHECK_EQUAL(decodedEndorse.getEndorseList().at(1), "homepage");
  BOOST_CHECK_EQUAL(decodedEndorse.getSigner(),
                    "/EndorseCertificateTests/EncodeDecode/ksk-1394072147335");
  BOOST_CHECK_EQUAL(decodedEndorse.getPublicKeyName(),
                    "/EndorseCertificateTests/EncodeDecode/ksk-1394072147335");
}

BOOST_AUTO_TEST_CASE(ConstructFromEndorseCert)
{
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

  boost::filesystem::path keyChainTmpPath =
    boost::filesystem::path(TEST_CERT_PATH) / "TestEndorseCertificate";
  KeyChain keyChain(std::string("sqlite3:").append(keyChainTmpPath.string()),
                    std::string("tpm-file:").append(keyChainTmpPath.string()));

  keyChain.signWithSha256(endorseCertificate);

  const Block& endorseDataBlock = endorseCertificate.wireEncode();

  Data decodedEndorseData;
  decodedEndorseData.wireDecode(endorseDataBlock);
  EndorseCertificate decodedEndorse(decodedEndorseData);
  BOOST_CHECK_EQUAL(decodedEndorse.getProfile().get("IDENTITY"),
                    "/EndorseCertificateTests/EncodeDecode");
  BOOST_CHECK_EQUAL(decodedEndorse.getProfile().get("name"), "MyName");
  BOOST_CHECK_EQUAL(decodedEndorse.getProfile().get("homepage"), "MyHomePage");
  BOOST_CHECK_EQUAL(decodedEndorse.getEndorseList().size(), 3);
  BOOST_CHECK_EQUAL(decodedEndorse.getEndorseList().at(0), "institution");
  BOOST_CHECK_EQUAL(decodedEndorse.getEndorseList().at(1), "group");
  BOOST_CHECK_EQUAL(decodedEndorse.getEndorseList().at(2), "advisor");
  BOOST_CHECK_EQUAL(decodedEndorse.getSigner(),
                    "/EndorseCertificateTests/Singer/ksk-1234567890");
  BOOST_CHECK_EQUAL(decodedEndorse.getPublicKeyName(),
                    "/EndorseCertificateTests/EncodeDecode/ksk-1394072147335");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace chronochat
