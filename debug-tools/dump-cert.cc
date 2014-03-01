/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013 Regents of the University of California.
 * @author: Yingdi Yu <yingdi@cs.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#include <ndn-cpp-dev/security/key-chain.hpp>
#include <ndn-cpp-dev/face.hpp>

using namespace ndn;

int 
main()
{
  Name root("/ndn");
  Name test("/ndn/test");
  Name alice("/ndn/test/alice");
  Name bob("/ndn/test/bob");
  Name cathy("/ndn/test/cathy");

  KeyChainImpl<SecPublicInfoSqlite3, SecTpmFile> keyChain;

  if(!keyChain.doesIdentityExist(root)
     || !keyChain.doesIdentityExist(test)
     || !keyChain.doesIdentityExist(alice)
     || !keyChain.doesIdentityExist(bob)
     || !keyChain.doesIdentityExist(cathy))
    return 1;

  shared_ptr<boost::asio::io_service> ioService = make_shared<boost::asio::io_service>();
  shared_ptr<Face> face = shared_ptr<Face>(new Face(ioService));
  // shared_ptr<Face> face = make_shared<Face>();
  
  face->put(*keyChain.getCertificate(keyChain.getDefaultCertificateNameForIdentity(test)));
  face->put(*keyChain.getCertificate(keyChain.getDefaultCertificateNameForIdentity(alice)));
  face->put(*keyChain.getCertificate(keyChain.getDefaultCertificateNameForIdentity(bob)));
  face->put(*keyChain.getCertificate(keyChain.getDefaultCertificateNameForIdentity(cathy)));

  ioService->run();
}
