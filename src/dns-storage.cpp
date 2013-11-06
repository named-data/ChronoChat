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
#include "exception.h"

#include <boost/filesystem.hpp>
#include "logging.h"


using namespace std;
using namespace ndn;
namespace fs = boost::filesystem;

INIT_LOGGER("DnsStorage");

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
    throw LnException("Chronos DNS DB cannot be open/created");

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
        throw LnException("Init \"error\" in DnsData");
    }
}

DnsStorage::~DnsStorage()
{
  sqlite3_close(m_db);
}

void
DnsStorage::updateDnsData(const ndn::Blob& data, const std::string& identity, const std::string& name, const std::string& type, const string& dataName)
{  
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT data_name FROM DnsData where dns_identity=? and dns_name=? and dns_type=?", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, identity.c_str(), identity.size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, name.c_str(), name.size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, type.c_str(), type.size(), SQLITE_TRANSIENT);

  if(sqlite3_step (stmt) != SQLITE_ROW)
    {
      _LOG_DEBUG("INSERT");
      sqlite3_finalize(stmt);
      sqlite3_prepare_v2 (m_db, "INSERT INTO DnsData (dns_identity, dns_name, dns_type, dns_value, data_name) VALUES (?, ?, ?, ?, ?)", -1, &stmt, 0);
      sqlite3_bind_text(stmt, 1, identity.c_str(), identity.size(), SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 2, name.c_str(), name.size(), SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 3, type.c_str(), type.size(), SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 4, data.buf(), data.size(), SQLITE_TRANSIENT); 
      sqlite3_bind_text(stmt, 5, dataName.c_str(), dataName.size(), SQLITE_TRANSIENT);
      sqlite3_step(stmt);
      sqlite3_finalize(stmt);
    }
  else
    {
      _LOG_DEBUG("UPDATE");
      sqlite3_finalize(stmt);
      sqlite3_prepare_v2 (m_db, "UPDATE DnsData SET dns_value=?, data_name=? WHERE dns_identity=? and dns_name=?, dns_type=?", -1, &stmt, 0);
      sqlite3_bind_text(stmt, 1, data.buf(), data.size(), SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 2, dataName.c_str(), dataName.size(), SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 3, identity.c_str(), identity.size(), SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 4, name.c_str(), name.size(), SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 5, type.c_str(), type.size(), SQLITE_TRANSIENT);
      sqlite3_step(stmt);
      sqlite3_finalize(stmt);
    }
}

void
DnsStorage::updateDnsSelfProfileData(const Data& data, const Name& identity)
{
  string dnsIdentity = identity.toUri();
  string dnsName("N/A");
  string dnsType("PROFILE");
  Ptr<Blob> dnsValue = data.encodeToWire();

  updateDnsData(*dnsValue, dnsIdentity, dnsName, dnsType, data.getName().toUri());
}

void
DnsStorage::updateDnsEndorseOthers(const Data& data, const Name& identity, const Name& endorsee)
{
  string dnsIdentity = identity.toUri();
  string dnsName = endorsee.toUri();
  string dnsType("ENDORSEE");
  Ptr<Blob> dnsValue = data.encodeToWire();

  updateDnsData(*dnsValue, dnsIdentity, dnsName, dnsType, data.getName().toUri());
}
  
void
DnsStorage::updateDnsOthersEndorse(const Data& data, const Name& identity)
{
  string dnsIdentity = identity.toUri();
  string dnsName("N/A");
  string dnsType("ENDORSED");
  Ptr<Blob> dnsValue = data.encodeToWire();

  updateDnsData(*dnsValue, dnsIdentity, dnsName, dnsType, data.getName().toUri());
}

Ptr<Data>
DnsStorage::getData(const Name& dataName)
{
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT dns_value FROM DnsData where data_name=?", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, dataName.toUri().c_str(), dataName.toUri().size(), SQLITE_TRANSIENT);
  
  if(sqlite3_step (stmt) == SQLITE_ROW)
    {
      Blob dnsDataBlob(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0));
      boost::iostreams::stream
        <boost::iostreams::array_source> is (dnsDataBlob.buf(), dnsDataBlob.size());
      return Data::decodeFromWire(is);
    }
  return NULL;
}
