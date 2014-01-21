/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013 Regents of the University of California.
 * @author: Yingdi Yu <yingdi@cs.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreorder"
#pragma clang diagnostic ignored "-Wtautological-compare"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-function"
#elif __GNUC__
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#include <boost/test/unit_test.hpp>

#include <iostream>
#include <iomanip>
#include <cryptopp/base64.h>
#include <ndn-cpp/security/key-chain.hpp>
#include "endorse-certificate.h"

using namespace ndn;
using namespace ndn::ptr_lib;
using namespace std;

BOOST_AUTO_TEST_SUITE(EndorseCertificateTests)

const string aliceCert("BIICqgOyEIUBbIk+NUYUb6QYOBmPyWrhj9WiEKgoAzXAjQ+gz6iAmeX4srvcH3dQ\
a/Y1H4Vf/LmNrXUsqbEJn1tmGoxGoQlkMtuKZ9K9X40R2SbH8d01IOIHILodRdG3\
KXlsXS13vBkuGB8RwhMCh3RCBc6K10LQ87TkkpLdYpIjS8n2stQn2HgiHPsIUGyE\
yLXqJ8ght2ZLVUYzHcpW4D0asckiQGXJuFmkGnbQVNHEBuirY8R1Zak6uDopoZi1\
xvFCH6UkUXBzh3FhXrk/GA5Y6sZTKTDqo1fwz6GRubyhPq1+nHOM7ud5k4DoC6zI\
Xstlhi5LSrCInSBItBaSNd2RPO3dbp0QAADy+p1uZG4A+sV1Y2xhLmVkdQD6nUtF\
WQD6rWFsaWNlAPr1a3NrLTEzODI5MzQyMDEA+r1JRC1DRVJUAPrN/f/////ea6GX\
AAABogPiAoX4UXq6YdztiNp79l71bcPBOBnRKOJBPKxDZTeC3YrfSgACurUFJt5r\
oZgAAeIB6vL6nW5kbgD6xXVjbGEuZWR1APqdS0VZAPr1ZHNrLTEzODI5MzQyMDAA\
+r1JRC1DRVJUAAAAAAABmhfdMIIBdzAiGA8yMDEzMTAyODAwMDAwMFoYDzIwMzMx\
MDI4MDAwMDAwWjArMCkGA1UEKRMiL25kbi91Y2xhLmVkdS9hbGljZS9rc2stMTM4\
MjkzNDIwMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAJ9b5S6MJIDz\
EyYDYL6/1PeeM1ZIr9lyDJojyLBa/nxvCkTykdeUEIEsQb0+B5UDcyS9iGZRlBUf\
CXOjRlnbo5BtD0IWw2aR048RT3pDh2U+dsOsJQMdPwTAaegwv+BEBvWVX+A93xya\
qoYQcJF56q6ktvxfFj5c4G6vuuuf8ZSGbIeesXy1P5wYdSu1ceTL8mnawR0+Nj2D\
VG71gn1A9NiIBKKQcT0rUgxo3NOaPHBUdQP67qLdfBOj0HrktGndVDxD5pWbnxKU\
V3zd/aD1DJuP826RJ8b/7eftdIF7/8gVRN5fz3wbjFtNzAE5RLmXP+ik8MhnHMx+\
B2QkdwR+8gcCAwEAAQAA");

KeyChain keyChain;

BOOST_AUTO_TEST_CASE(ProfileDataEncodeDecode)
{
  string decoded;
  CryptoPP::StringSource ss(reinterpret_cast<const unsigned char *>(aliceCert.c_str()), 
                            aliceCert.size(), 
                            true,
                            new CryptoPP::Base64Decoder(new CryptoPP::StringSink(decoded)));
  Data data;
  data.wireDecode(Block(decoded.c_str(), decoded.size()));
  IdentityCertificate identityCertificate(data);
  
  Profile profile(identityCertificate);
  ProfileData profileData(profile);

  Name certificateName = keyChain.getDefaultCertificateName();
  keyChain.sign(profileData, certificateName);

  const Block& profileDataBlock = profileData.wireEncode();
  Data decodedProfileData;
  
  decodedProfileData.wireDecode(profileDataBlock);
  ProfileData decodedProfile(decodedProfileData);
  BOOST_CHECK_EQUAL(decodedProfile.getProfile().getProfileEntry("IDENTITY"), string("/ndn/ucla.edu/alice"));
  BOOST_CHECK_EQUAL(decodedProfile.getProfile().getProfileEntry("name"), string("/ndn/ucla.edu/alice/ksk-1382934201"));
}

BOOST_AUTO_TEST_CASE(EndorseCertifiicateEncodeDecode)
{
  string decoded;
  CryptoPP::StringSource ss(reinterpret_cast<const unsigned char *>(aliceCert.c_str()), 
                            aliceCert.size(), 
                            true,
                            new CryptoPP::Base64Decoder(new CryptoPP::StringSink(decoded)));
  Data data;
  data.wireDecode(Block(decoded.c_str(), decoded.size()));
  IdentityCertificate identityCertificate(data);

  Profile profile(identityCertificate);
  ProfileData profileData(profile);
  
  Name certificateName = keyChain.getDefaultCertificateName();
  keyChain.sign(profileData, certificateName);

  EndorseCertificate endorseCertificate(identityCertificate, profileData);

  keyChain.sign(endorseCertificate, certificateName);
  
  const Block& endorseDataBlock = endorseCertificate.wireEncode();

  Data decodedEndorseData;

  decodedEndorseData.wireDecode(endorseDataBlock);
  EndorseCertificate decodedEndorse(decodedEndorseData);
  BOOST_CHECK_EQUAL(decodedEndorse.getProfileData().getProfile().getProfileEntry("IDENTITY"), string("/ndn/ucla.edu/alice"));
  BOOST_CHECK_EQUAL(decodedEndorse.getProfileData().getProfile().getProfileEntry("name"), string("/ndn/ucla.edu/alice/ksk-1382934201"));
}



BOOST_AUTO_TEST_SUITE_END()

