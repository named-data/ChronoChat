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

  KeyChain keyChain("sqlite3", "file");

  if(!keyChain.doesIdentityExist(root))
    return 1;

  Face face;

  Name name("/local/ndn/prefix");
  name.appendVersion().appendSegment(0);

  shared_ptr<Data> data = make_shared<Data>(name);
  std::string prefix("/ndn/test");
  data->setContent(reinterpret_cast<const uint8_t*>(prefix.c_str()), prefix.size());
  keyChain.signByIdentity(*data, root);

  face.put(*data);

  face.getIoService().run();
}
