/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013 Regents of the University of California.
 * @author: Yingdi Yu <yingdi@cs.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#include "common.hpp"
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/face.hpp>

using namespace ndn;

int
main()
{
  Name root("/ndn");
  Name test("/ndn/test");
  Name alice("/ndn/test/alice");
  Name bob("/ndn/test/bob");
  Name cathy("/ndn/test/cathy");

  KeyChain keyChain("sqlite3", "file");

  if (!keyChain.doesIdentityExist(root) ||
      !keyChain.doesIdentityExist(test) ||
      !keyChain.doesIdentityExist(alice) ||
      !keyChain.doesIdentityExist(bob) ||
      !keyChain.doesIdentityExist(cathy))
    return 1;

  Face face;

  face.put(*keyChain.getCertificate(keyChain.getDefaultCertificateNameForIdentity(test)));
  face.put(*keyChain.getCertificate(keyChain.getDefaultCertificateNameForIdentity(alice)));
  face.put(*keyChain.getCertificate(keyChain.getDefaultCertificateNameForIdentity(bob)));
  face.put(*keyChain.getCertificate(keyChain.getDefaultCertificateNameForIdentity(cathy)));

  face.getIoService().run();
}
