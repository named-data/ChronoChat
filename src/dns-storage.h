/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOS_DNS_STORAGE_H
#define CHRONOS_DNS_STORAGE_H

#include "config.h"
#include <sqlite3.h>
#include <ndn-cpp-dev/data.hpp>

namespace chronos{

class DnsStorage
{
public:
  struct Error : public std::runtime_error { Error(const std::string &what) : std::runtime_error(what) {} };

  DnsStorage();

  ~DnsStorage()
  { sqlite3_close(m_db); }

  void
  updateDnsSelfProfileData(const ndn::Data& data, const ndn::Name& identity)
  { updateDnsData(data.wireEncode(), identity.toUri(), "N/A", "PROFILE", data.getName().toUri()); }

  void
  updateDnsEndorseOthers(const ndn::Data& data, const ndn::Name& identity, const ndn::Name& endorsee)
  { updateDnsData(data.wireEncode(), identity.toUri(), endorsee.toUri(), "ENDORSEE", data.getName().toUri()); }
  
  void
  updateDnsOthersEndorse(const ndn::Data& data, const ndn::Name& identity)
  { updateDnsData(data.wireEncode(), identity.toUri(), "N/A", "ENDORSED", data.getName().toUri()); }

  ndn::ptr_lib::shared_ptr<ndn::Data>
  getData(const ndn::Name& name);

private:
  void
  updateDnsData(const ndn::Block& data, const std::string& identity, const std::string& name, const std::string& type, const std::string& dataName);

private:
  sqlite3 *m_db;
};

}//chronos

#endif
