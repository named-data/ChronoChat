/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "dns-storage.h"

#include <boost/filesystem.hpp>
#include "logging.h"


using namespace std;
using namespace ndn;
using namespace ndn::ptr_lib;
namespace fs = boost::filesystem;

INIT_LOGGER("DnsStorage");

namespace chronos{

const string INIT_DD_TABLE = "\
CREATE TABLE IF NOT EXISTS                                           \n \
  DnsData(                                                           \n \
      dns_identity  BLOB NOT NULL,                                   \n \
      dns_name      BLOB NOT NULL,                                   \n \
      dns_type      BLOB NOT NULL,                                   \n \
      data_name     BLOB NOT NULL,                                   \n	\
      dns_value     BLOB NOT NULL,                                   \n \
                                                                     \
      PRIMARY KEY (dns_identity, dns_name, dns_type)                 \n \
  );                                                                 \
CREATE INDEX dd_index ON DnsData(dns_identity, dns_name, dns_type);  \n \
CREATE INDEX dd_index2 ON DnsData(data_name);                        \n \
";

DnsStorage::DnsStorage()
{
  fs::path chronosDir = fs::path(getenv("HOME")) / ".chronos";
  fs::create_directories (chronosDir);

  int res = sqlite3_open((chronosDir / "dns.db").c_str (), &m_db);
  if (res != SQLITE_OK)
    throw Error("Chronos DNS DB cannot be open/created");

  // Check if SelfProfile table exists
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT name FROM sqlite_master WHERE type='table' And name='DnsData'", -1, &stmt, 0);
  res = sqlite3_step (stmt);

  bool ddTableExist = false;
  if (res == SQLITE_ROW)
      ddTableExist = true;
  sqlite3_finalize (stmt);

  if(!ddTableExist)
    {
      char *errmsg = 0;
      res = sqlite3_exec (m_db, INIT_DD_TABLE.c_str (), NULL, NULL, &errmsg);
      if (res != SQLITE_OK && errmsg != 0)
        throw Error("Init \"error\" in DnsData");
    }
}

void
DnsStorage::updateDnsData(const ndn::Block& data, const std::string& identity, const std::string& name, const std::string& type, const string& dataName)
{  
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "INSERT OR REPLACE INTO DnsData (dns_identity, dns_name, dns_type, dns_value, data_name) VALUES (?, ?, ?, ?, ?)", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, identity.c_str(), identity.size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, name.c_str(), name.size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, type.c_str(), type.size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 4, (const char*)data.wire(), data.size(), SQLITE_TRANSIENT); 
  sqlite3_bind_text(stmt, 5, dataName.c_str(), dataName.size(), SQLITE_TRANSIENT);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
}

shared_ptr<Data>
DnsStorage::getData(const Name& dataName)
{
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT dns_value FROM DnsData where data_name=?", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, dataName.toUri().c_str(), dataName.toUri().size(), SQLITE_TRANSIENT);
  
  if(sqlite3_step (stmt) == SQLITE_ROW)
    {
      shared_ptr<Data> data = make_shared<Data>();
      data->wireDecode(Block(reinterpret_cast<const uint8_t*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0)));
      sqlite3_finalize(stmt);
      return data;
    }
  sqlite3_finalize(stmt);

  return shared_ptr<Data>();
}

}//chronos
