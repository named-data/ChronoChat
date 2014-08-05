/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013 Regents of the University of California.
 * @author: Yingdi Yu <yingdi@cs.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#include <ndn-cxx/security/key-chain.hpp>

using namespace ndn;

int
main()
{
  KeyChain keyChain("sqlite3", "file");
  std::vector<CertificateSubjectDescription> subjectDescription;

  Name root("/ndn");
  Name rootCertName = keyChain.createIdentity(root);

  Name test("/ndn/test");
  Name testKeyName = keyChain.generateRsaKeyPairAsDefault(test, true);
  shared_ptr<IdentityCertificate> testCert =
    keyChain.prepareUnsignedIdentityCertificate(testKeyName, root,
                                                time::system_clock::now(),
                                                time::system_clock::now() + time::days(7300),
                                                subjectDescription);
  keyChain.signByIdentity(*testCert, root);
  keyChain.addCertificateAsIdentityDefault(*testCert);

  Name alice("/ndn/test/alice");
  if(!keyChain.doesIdentityExist(alice))
    {
      Name aliceKeyName = keyChain.generateRsaKeyPairAsDefault(alice, true);
      shared_ptr<IdentityCertificate> aliceCert =
        keyChain.prepareUnsignedIdentityCertificate(aliceKeyName, test,
                                                    time::system_clock::now(),
                                                    time::system_clock::now() + time::days(7300),
                                                    subjectDescription);
      keyChain.signByIdentity(*aliceCert, test);
      keyChain.addCertificateAsIdentityDefault(*aliceCert);
    }

  Name bob("/ndn/test/bob");
  if(!keyChain.doesIdentityExist(bob))
    {
      Name bobKeyName = keyChain.generateRsaKeyPairAsDefault(bob, true);
      shared_ptr<IdentityCertificate> bobCert =
        keyChain.prepareUnsignedIdentityCertificate(bobKeyName, test,
                                                    time::system_clock::now(),
                                                    time::system_clock::now() + time::days(7300),
                                                    subjectDescription);
      keyChain.signByIdentity(*bobCert, test);
      keyChain.addCertificateAsIdentityDefault(*bobCert);
    }

  Name cathy("/ndn/test/cathy");
  if(!keyChain.doesIdentityExist(cathy))
    {
      Name cathyKeyName = keyChain.generateRsaKeyPairAsDefault(cathy, true);
      shared_ptr<IdentityCertificate> cathyCert =
        keyChain.prepareUnsignedIdentityCertificate(cathyKeyName, test,
                                                    time::system_clock::now(),
                                                    time::system_clock::now() + time::days(7300),
                                                    subjectDescription);
      keyChain.signByIdentity(*cathyCert, test);
      keyChain.addCertificateAsIdentityDefault(*cathyCert);
    }
}
