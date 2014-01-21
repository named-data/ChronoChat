/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef LINKNDN_DNS_STORAGE_H
#define LINKNDN_DNS_STORAGE_H

#include <sqlite3.h>
#include <ndn-cpp-dev/data.hpp>

class DnsStorage
{
public:
  struct Error : public std::runtime_error { Error(const std::string &what) : std::runtime_error(what) {} };

  DnsStorage();

  ~DnsStorage();

  void
  updateDnsSelfProfileData(const ndn::Data& data, const ndn::Name& identity);

  void
  updateDnsEndorseOthers(const ndn::Data& data, const ndn::Name& identity, const ndn::Name& endorsee);
  
  void
  updateDnsOthersEndorse(const ndn::Data& data, const ndn::Name& identity);

  ndn::ptr_lib::shared_ptr<ndn::Data>
  getData(const ndn::Name& name);

private:
  void
  updateDnsData(const ndn::Block& data, const std::string& identity, const std::string& name, const std::string& type, const std::string& dataName);

private:
  sqlite3 *m_db;
};

#endif
