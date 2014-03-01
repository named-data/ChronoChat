/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013 Regents of the University of California.
 * @author: Yingdi Yu <yingdi@cs.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#include <ndn-cpp-dev/security/key-chain.hpp>

using namespace ndn;

int 
main()
{
  KeyChainImpl<SecPublicInfoSqlite3, SecTpmFile> keyChain;
  std::vector<CertificateSubjectDescription> subjectDescription;

  Name root("/ndn");
  Name rootCertName = keyChain.createIdentity(root);

  Name test("/ndn/test");
  Name testKeyName = keyChain.generateRSAKeyPairAsDefault(test, true);
  shared_ptr<IdentityCertificate> testCert = 
    keyChain.prepareUnsignedIdentityCertificate(testKeyName, root, getNow(), getNow() + 630720000, subjectDescription);
  keyChain.signByIdentity(*testCert, root);
  keyChain.addCertificateAsIdentityDefault(*testCert);

  Name alice("/ndn/test/alice");
  if(!keyChain.doesIdentityExist(alice))
    {
      Name aliceKeyName = keyChain.generateRSAKeyPairAsDefault(alice, true);
      shared_ptr<IdentityCertificate> aliceCert = 
	keyChain.prepareUnsignedIdentityCertificate(aliceKeyName, test, getNow(), getNow() + 630720000, subjectDescription);
      keyChain.signByIdentity(*aliceCert, test);
      keyChain.addCertificateAsIdentityDefault(*aliceCert);
    }

  Name bob("/ndn/test/bob");
  if(!keyChain.doesIdentityExist(bob))
    {
      Name bobKeyName = keyChain.generateRSAKeyPairAsDefault(bob, true);
      shared_ptr<IdentityCertificate> bobCert = 
	keyChain.prepareUnsignedIdentityCertificate(bobKeyName, test, getNow(), getNow() + 630720000, subjectDescription);
      keyChain.signByIdentity(*bobCert, test);
      keyChain.addCertificateAsIdentityDefault(*bobCert);
    }

  Name cathy("/ndn/test/cathy");
  if(!keyChain.doesIdentityExist(cathy))
    {
      Name cathyKeyName = keyChain.generateRSAKeyPairAsDefault(cathy, true);
      shared_ptr<IdentityCertificate> cathyCert = 
	keyChain.prepareUnsignedIdentityCertificate(cathyKeyName, test, getNow(), getNow() + 630720000, subjectDescription);
      keyChain.signByIdentity(*cathyCert, test);
      keyChain.addCertificateAsIdentityDefault(*cathyCert);
    }
}
