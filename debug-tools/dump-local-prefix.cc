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

  KeyChainImpl<SecPublicInfoSqlite3, SecTpmFile> keyChain;

  if(!keyChain.doesIdentityExist(root))
    return 1;

  shared_ptr<boost::asio::io_service> ioService = make_shared<boost::asio::io_service>();
  shared_ptr<Face> face = shared_ptr<Face>(new Face(ioService));

  Name name("/local/ndn/prefix");
  name.appendVersion().appendSegment(0);

  Data data(name);
  std::string prefix("/ndn/test");
  data.setContent(reinterpret_cast<const uint8_t*>(prefix.c_str()), prefix.size());
  keyChain.signByIdentity(data, root);
  
  face->put(data);

  ioService->run();
}
