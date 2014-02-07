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

#include <cryptopp/base64.h>
#include <ndn-cpp-dev/security/key-chain.hpp>
#include <ndn-cpp-dev/util/time.hpp>
#include "endorse-certificate.h"

using namespace ndn;
using namespace std;
using namespace chronos;

BOOST_AUTO_TEST_SUITE(TestEndorseCertificate)

const string testCert("Av0DgANRBBdFbmRvcnNlQ2VydGlmaWNhdGVUZXN0cwQMRW5jb2RlRGVjb2RlBANL\
RVkEEWtzay0xMzkxNzM0NzY0MzE2BAdJRC1DRVJUBAf9AUQJ9fu/EAMUAQIR/QHP\
MIIByzAiGA8yMDE0MDEwMTAwMDAwMFoYDzIwMTUwMTAxMDAwMDAwWjB/MCwGA1UE\
KRMlL0VuZG9yc2VDZXJ0aWZpY2F0ZVRlc3RzL0VuY29kZURlY29kZTASBgkqhkiG\
9w0BCQETBWFAYi5jMAoGA1UECxMDbmRuMA4GA1UEARMHdWNsYSBjczATBgNVBAMT\
DGh0dHA6Ly9hLmIuYzAKBgNVBFATA25kbjCCASIwDQYJKoZIhvcNAQEBBQADggEP\
ADCCAQoCggEBALX3RJhGUUQp+i6nJRMWgCbCxxrpmSOGlUfLNAFQh/zizdFdpG0O\
+fjI82ilUzOsi4KnCBxpEez+tEMFzRTOFaLYj6bGQhTKhMsDb/d5mtjjKRBE69+R\
H4IC9rUsD+pD/qh4lEZwoxz74DmsbAL4sCXyNLOTJFAokKpD5GB5/enwkMyC8425\
seQXCZw/rlhpeIZkh19ULOvq4GXPk6fVPW/HsnReNa2J8toQepoh5nk1KAZjc/ed\
GdegTAn4rBUH65chfoztBMli7sE9BJMkVf0OHSXTbRGqGSR/u0uOs+F69QisS+du\
xyEZJFqJGOv7ZROo9C6cRZFpCJH+pjLen00CAwEAARJPFgEBF0oDSAQXRW5kb3Jz\
ZUNlcnRpZmljYXRlVGVzdHMEDEVuY29kZURlY29kZQQDS0VZBBFrc2stMTM5MTcz\
NDc2NDMxNgQHSUQtQ0VSVBP9AQBOUc/C6yrGQ9A3FYlwklNz9WrK7FhvkMmynugY\
Ejasc6rVeMgigCIwgLau+bnIbO0rfMLSGdrvB/XSiWFzZSt1fQj/uJ29he+tIBf+\
B5tP0MJL5hFNtSxgHgMCXkSRl9cC356GPpEcG+vMCoYMQLILukXfTwhP8qUB/hoy\
5/1OxXMUAFnU2EoX90DJYvpVlD6nZt04okLVBUpNJSODEzFJrNbgpapzkb4djGol\
6uRDV8iGhMfFtCmf+ZtcXyr49uBF2npqwgz01lTnXAB6jExD+EQ9rxospGkMgKVz\
camQA/xj3G2PdPylszVO6qfv2clQxr9atW2Vt1BeI1ZtFbd/");

KeyChain keyChain;

BOOST_AUTO_TEST_CASE(EncodeDecode)
{
  string decoded;
  CryptoPP::StringSource ss(reinterpret_cast<const uint8_t*>(testCert.c_str()), 
                            testCert.size(), true,
                            new CryptoPP::Base64Decoder(new CryptoPP::StringSink(decoded)));
  Data data;
  data.wireDecode(Block(decoded.c_str(), decoded.size()));
  IdentityCertificate identityCertificate(data);

  Profile profile(identityCertificate);
  vector<string> endorseList;
  endorseList.push_back("email");
  endorseList.push_back("homepage");

  EndorseCertificate endorseCertificate(identityCertificate, profile, endorseList);

  KeyChainImpl<SecPublicInfoSqlite3, SecTpmFile> keyChain;
  Name signingIdentity("/EndorseCertificateTests/EncodeDecode/"+boost::lexical_cast<string>(time::now()));
  keyChain.createIdentity(signingIdentity);

  keyChain.signByIdentity(endorseCertificate, signingIdentity);
  
  const Block& endorseDataBlock = endorseCertificate.wireEncode();

  Data decodedEndorseData;

  decodedEndorseData.wireDecode(endorseDataBlock);
  EndorseCertificate decodedEndorse(decodedEndorseData);
  BOOST_CHECK_EQUAL(decodedEndorse.getProfile().get("IDENTITY"), "/EndorseCertificateTests/EncodeDecode");
  BOOST_CHECK_EQUAL(decodedEndorse.getProfile().get("name"), "/EndorseCertificateTests/EncodeDecode");
  BOOST_CHECK_EQUAL(decodedEndorse.getProfile().get("homepage"), "http://a.b.c");
  BOOST_CHECK_EQUAL(decodedEndorse.getEndorseList().size(), 2);
  BOOST_CHECK_EQUAL(decodedEndorse.getEndorseList().at(0), "email");
  BOOST_CHECK_EQUAL(decodedEndorse.getEndorseList().at(1), "homepage");

  keyChain.deleteIdentity(signingIdentity);
}



BOOST_AUTO_TEST_SUITE_END()

