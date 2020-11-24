/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2020 Regents of the University of California.
 * @author: Yingdi Yu <yingdi@cs.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/util/io.hpp>
#include <iostream>

using namespace ndn;
using ndn::security::Certificate;
using ndn::security::pib::Identity;
using ndn::security::pib::Key;

void
generateCertificate(KeyChain& keyChain, Name name) {
  try {
    keyChain.getPib().getIdentity(name);
  } catch (const ndn::security::pib::Pib::Error&) {
    Identity id = keyChain.createIdentity(name);
    Key key = id.getDefaultKey();
    Certificate cert = Certificate(key.getDefaultCertificate());

    ndn::SignatureInfo signatureInfo;
    signatureInfo.setValidityPeriod(ndn::security::ValidityPeriod(
      time::system_clock::now(), time::system_clock::now() + time::days(7300)));

    keyChain.sign(cert, security::signingByIdentity("/ndn/test").setSignatureInfo(signatureInfo));
    keyChain.setDefaultCertificate(key, cert);

    std::cout << "Generated cert " << cert.getName() << " with KeyLocator " << cert.getKeyLocator().value() << std::endl;
  }
}

int
main()
{
  KeyChain keyChain;

  // Root certificate
  generateCertificate(keyChain, "/ndn/test");

  // Test certificates
  generateCertificate(keyChain, "/ndn/test/alice");
  generateCertificate(keyChain, "/ndn/test/bob");
  generateCertificate(keyChain, "/ndn/test/cathy");

  std::ofstream file("security/test-anchor.cert");
  ndn::io::saveBlock(keyChain.getPib().getIdentity("/ndn/test")
                             .getDefaultKey().getDefaultCertificate().wireEncode(),
                     file);
}
