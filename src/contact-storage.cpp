/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "contact-storage.h"

#include <boost/filesystem.hpp>
#include <cryptopp/sha.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>
#include "logging.h"

using namespace std;
using namespace ndn;
namespace fs = boost::filesystem;


INIT_LOGGER ("chronos.ContactStorage");

namespace chronos{

const string INIT_SP_TABLE = "\
CREATE TABLE IF NOT EXISTS                                           \n \
  SelfProfile(                                                       \n \
      profile_type      BLOB NOT NULL,                               \n \
      profile_value     BLOB NOT NULL,                               \n \
                                                                     \
      PRIMARY KEY (profile_type)                                     \n \
  );                                                                 \n \
                                                                     \
CREATE INDEX sp_index ON SelfProfile(profile_type);                  \n \
"; // user's own profile;

const string INIT_SE_TABLE = "\
CREATE TABLE IF NOT EXISTS                                           \n \
  SelfEndorse(                                                       \n \
      identity          BLOB NOT NULL UNIQUE,                        \n \
      endorse_data      BLOB NOT NULL,                               \n \
                                                                     \
      PRIMARY KEY (identity)                                         \n \
  );                                                                 \n \
CREATE INDEX se_index ON SelfEndorse(identity);                      \n \
"; // user's self endorse cert;

const string INIT_CONTACT_TABLE = "\
CREATE TABLE IF NOT EXISTS                                           \n \
  Contact(                                                           \n \
      contact_namespace BLOB NOT NULL,                               \n \
      contact_alias     BLOB NOT NULL,                               \n \
      contact_keyName   BLOB NOT NULL,                               \n \
      contact_key       BLOB NOT NULL,                               \n \
      notBefore         INTEGER DEFAULT 0,                           \n \
      notAfter          INTEGER DEFAULT 0,                           \n \
      is_introducer     INTEGER DEFAULT 0,                           \n \
                                                                     \
      PRIMARY KEY (contact_namespace)                                \n \
  );                                                                 \n \
                                                                     \
CREATE INDEX contact_index ON Contact(contact_namespace);            \n \
"; // contact's basic info

const string INIT_TS_TABLE = "\
CREATE TABLE IF NOT EXISTS                                           \n \
  TrustScope(                                                        \n \
      id                INTEGER PRIMARY KEY AUTOINCREMENT,           \n \
      contact_namespace BLOB NOT NULL,                               \n \
      trust_scope       BLOB NOT NULL                                \n \
  );                                                                 \n \
                                                                     \
CREATE INDEX ts_index ON TrustScope(contact_namespace);              \n \
"; // contact's trust scope;

const string INIT_CP_TABLE = "\
CREATE TABLE IF NOT EXISTS                                           \n \
  ContactProfile(                                                    \n \
      profile_identity  BLOB NOT NULL,                               \n \
      profile_type      BLOB NOT NULL,                               \n \
      profile_value     BLOB NOT NULL,                               \n \
      endorse           INTEGER NOT NULL,                            \n \
                                                                     \
      PRIMARY KEY (profile_identity, profile_type)                   \n \
  );                                                                 \n \
                                                                     \
CREATE INDEX cp_index ON ContactProfile(profile_identity);           \n \
"; // contact's profile

const string INIT_PE_TABLE = "\
CREATE TABLE IF NOT EXISTS                                           \n \
  ProfileEndorse(                                                    \n \
      identity          BLOB NOT NULL UNIQUE,                        \n \
      endorse_data      BLOB NOT NULL,                               \n \
                                                                     \
      PRIMARY KEY (identity)                                         \n \
  );                                                                 \n \
                                                                     \
CREATE INDEX pe_index ON ProfileEndorse(identity);                   \n \
"; // user's endorsement on contacts

const string INIT_CE_TABLE = "\
CREATE TABLE IF NOT EXISTS                                           \n \
  CollectEndorse(                                                    \n \
      endorser          BLOB NOT NULL,                               \n \
      endorse_name      BLOB NOT NULL,                               \n \
      endorse_data      BLOB NOT NULL,                               \n \
                                                                     \
      PRIMARY KEY (endorser)                                         \n \
  );                                                                 \n \
"; // contact's endorsements on the user

const string INIT_DD_TABLE = "\
CREATE TABLE IF NOT EXISTS                                           \n \
  DnsData(                                                           \n \
      dns_name      BLOB NOT NULL,                                   \n \
      dns_type      BLOB NOT NULL,                                   \n \
      data_name     BLOB NOT NULL,                                   \n	\
      dns_value     BLOB NOT NULL,                                   \n \
                                                                     \
      PRIMARY KEY (dns_name, dns_type)                               \n \
  );                                                                 \
CREATE INDEX dd_index ON DnsData(dns_name, dns_type);                \n \
CREATE INDEX dd_index2 ON DnsData(data_name);                        \n \
"; // dns data;

ContactStorage::ContactStorage(const Name& identity)
  : m_identity(identity)
{
  fs::path chronosDir = fs::path(getenv("HOME")) / ".chronos";
  fs::create_directories (chronosDir);

  int res = sqlite3_open((chronosDir / getDBName()).c_str (), &m_db);
  if (res != SQLITE_OK)
    throw Error("Chronos DB cannot be open/created");

  initializeTable("SelfProfile", INIT_SP_TABLE);
  initializeTable("SelfEndorse", INIT_SE_TABLE);
  initializeTable("Contact", INIT_CONTACT_TABLE);
  initializeTable("TrustScope", INIT_TS_TABLE);
  initializeTable("ContactProfile", INIT_CP_TABLE);
  initializeTable("ProfileEndorse", INIT_PE_TABLE);
  initializeTable("CollectEndorse", INIT_CE_TABLE);
  initializeTable("DnsData", INIT_DD_TABLE);

}

string
ContactStorage::getDBName()
{
  string dbName("chronos-");

  stringstream ss;
  {
    using namespace CryptoPP;

    SHA256 hash;
    StringSource(m_identity.wireEncode().wire(), m_identity.wireEncode().size(), true,
                 new HashFilter(hash, new HexEncoder(new FileSink(ss), false)));
  }
  dbName.append(ss.str()).append(".db");

  return dbName;
}

void
ContactStorage::initializeTable(const string& tableName, const string& sqlCreateStmt)
{
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT name FROM sqlite_master WHERE type='table' And name=?", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, tableName.c_str(), tableName.size(), SQLITE_TRANSIENT);
  int res = sqlite3_step (stmt);

  bool tableExist = false;
  if (res == SQLITE_ROW)
      tableExist = true;
  sqlite3_finalize (stmt);

  if(!tableExist)
    {
      char *errmsg = 0;
      res = sqlite3_exec (m_db, sqlCreateStmt.c_str (), NULL, NULL, &errmsg);
      if (res != SQLITE_OK && errmsg != 0)
        throw Error("Init \"error\" in " + tableName);
    }
}

shared_ptr<Profile>
ContactStorage::getSelfProfile()
{  
  shared_ptr<Profile> profile = make_shared<Profile>(m_identity);
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT profile_type, profile_value FROM SelfProfile", -1, &stmt, 0);

  while( sqlite3_step (stmt) == SQLITE_ROW)
    {
      string profileType(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0));
      string profileValue(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)), sqlite3_column_bytes (stmt, 1));
      (*profile)[profileType] = profileValue;
    }

  sqlite3_finalize(stmt);

  return profile;
}

void
ContactStorage::addSelfEndorseCertificate(const EndorseCertificate& newEndorseCertificate)
{
  const Block& newEndorseCertificateBlock = newEndorseCertificate.wireEncode();

  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "INSERT OR REPLACE INTO SelfEndorse (identity, endorse_data) values (?, ?)", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, m_identity.toUri().c_str(), m_identity.toUri().size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, reinterpret_cast<const char*>(newEndorseCertificateBlock.wire()), newEndorseCertificateBlock.size(), SQLITE_TRANSIENT);
  int res = sqlite3_step(stmt);

  sqlite3_finalize (stmt);
}

void
ContactStorage::addEndorseCertificate(const EndorseCertificate& endorseCertificate, const Name& identity)
{
  const Block& newEndorseCertificateBlock = endorseCertificate.wireEncode();

  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "INSERT OR REPLACE INTO ProfileEndorse (identity, endorse_data) values (?, ?)", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, identity.toUri().c_str(), identity.toUri().size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, reinterpret_cast<const char*>(newEndorseCertificateBlock.wire()), newEndorseCertificateBlock.size(), SQLITE_TRANSIENT);
  sqlite3_step(stmt);

  sqlite3_finalize (stmt);
}

void
ContactStorage::updateCollectEndorse(const EndorseCertificate& endorseCertificate)
{
  Name endorserName = endorseCertificate.getSigner();
  Name certName = endorseCertificate.getName();

  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "INSERT OR REPLACE INTO CollectEndorse (endorser, endorse_name, endorse_data) VALUES (?, ?, ?, ?)", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, endorserName.toUri().c_str(), endorserName.toUri().size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, certName.toUri().c_str(), certName.toUri().size(), SQLITE_TRANSIENT);
  const Block &block = endorseCertificate.wireEncode();
  sqlite3_bind_text(stmt, 3, reinterpret_cast<const char*>(block.wire()), block.size(), SQLITE_TRANSIENT);
  int res = sqlite3_step (stmt);
  sqlite3_finalize (stmt); 
  return;
}

void
ContactStorage::getCollectEndorse(EndorseCollection& endorseCollection)
{
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT endorse_name, endorse_data FROM CollectEndorse", -1, &stmt, 0);

  while(sqlite3_step (stmt) == SQLITE_ROW)
    {
      string certName(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes(stmt, 1));
      stringstream ss;
      {
        using namespace CryptoPP;
        SHA256 hash;

        StringSource(sqlite3_column_text(stmt, 1), sqlite3_column_bytes (stmt, 0), true,
                     new HashFilter(hash, new FileSink(ss)));
      }
      EndorseCollection::Endorsement* endorsement = endorseCollection.add_endorsement();
      endorsement->set_certname(certName);
      endorsement->set_hash(ss.str());
    }

  sqlite3_finalize (stmt);
}

void
ContactStorage::getEndorseList(const Name& identity, vector<string>& endorseList)
{
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT profile_type FROM ContactProfile WHERE profile_identity=? AND endorse=1 ORDER BY profile_type", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, identity.toUri().c_str(), identity.toUri().size(), SQLITE_TRANSIENT);

  while( sqlite3_step (stmt) == SQLITE_ROW)
    {
      string profileType(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0));
      endorseList.push_back(profileType);      
    }
  sqlite3_finalize (stmt);  
}


void 
ContactStorage::removeContact(const Name& identityName)
{
  string identity = identityName.toUri();
  
  sqlite3_stmt *stmt;  
  sqlite3_prepare_v2 (m_db, "DELETE FROM Contact WHERE contact_namespace=?", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, identity.c_str(), identity.size (), SQLITE_TRANSIENT);
  int res = sqlite3_step (stmt);
  sqlite3_finalize (stmt);

  sqlite3_prepare_v2 (m_db, "DELETE FROM ContactProfile WHERE profile_identity=?", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, identity.c_str(),  identity.size (), SQLITE_TRANSIENT);
  res = sqlite3_step (stmt);
  sqlite3_finalize (stmt);
  
  sqlite3_prepare_v2 (m_db, "DELETE FROM TrustScope WHERE contact_namespace=?", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, identity.c_str(),  identity.size (), SQLITE_TRANSIENT);
  res = sqlite3_step (stmt);
  sqlite3_finalize (stmt);         
}
  
void
ContactStorage::addContact(const Contact& contact)
{
  if(doesContactExist(contact.getNameSpace()))
    throw Error("Normal Contact has already existed");

  string identity = contact.getNameSpace().toUri();
  bool isIntroducer = contact.isIntroducer();

  sqlite3_stmt *stmt;  
  sqlite3_prepare_v2 (m_db, 
                      "INSERT INTO Contact (contact_namespace, contact_alias, contact_keyName, contact_key, notBefore, notAfter, is_introducer) values (?, ?, ?, ?, ?, ?, ?)", 
                      -1,
                      &stmt,
                      0);

  sqlite3_bind_text(stmt, 1, identity.c_str(), identity.size (), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, contact.getAlias().c_str(), contact.getAlias().size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, contact.getPublicKeyName().toUri().c_str(), contact.getPublicKeyName().toUri().size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 4, reinterpret_cast<const char*>(contact.getPublicKey().get().buf()), contact.getPublicKey().get().size(), SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 5, time::toUnixTimestamp(contact.getNotBefore()).count());
  sqlite3_bind_int64(stmt, 6, time::toUnixTimestamp(contact.getNotAfter()).count());
  sqlite3_bind_int(stmt, 7, (isIntroducer ? 1 : 0));

  int res = sqlite3_step (stmt);

  // _LOG_DEBUG("addContact: " << res);

  sqlite3_finalize (stmt);

  const Profile& profile = contact.getProfile();
  Profile::const_iterator it = profile.begin();

  for(; it != profile.end(); it++)
    {
      sqlite3_prepare_v2 (m_db, 
                          "INSERT INTO ContactProfile (profile_identity, profile_type, profile_value, endorse) values (?, ?, ?, 0)", 
                          -1, 
                          &stmt, 
                          0);
      sqlite3_bind_text(stmt, 1, identity.c_str(),  identity.size (), SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 2, it->first.c_str(), it->first.size(), SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 3, it->second.c_str(), it->second.size(), SQLITE_TRANSIENT);
      res = sqlite3_step (stmt);
      sqlite3_finalize (stmt);
    }

  if(isIntroducer)
    {
      Contact::const_iterator it  = contact.trustScopeBegin();
      Contact::const_iterator end = contact.trustScopeEnd();

      while(it != end)
        {
          sqlite3_prepare_v2 (m_db, 
                              "INSERT INTO TrustScope (contact_namespace, trust_scope) values (?, ?)", 
                              -1, 
                              &stmt, 
                              0);
          sqlite3_bind_text(stmt, 1, identity.c_str(), identity.size (), SQLITE_TRANSIENT);
          sqlite3_bind_text(stmt, 2, it->first.toUri().c_str(), it->first.toUri().size(), SQLITE_TRANSIENT);
          res = sqlite3_step (stmt);
          sqlite3_finalize (stmt);          
          it++;
        }
    }
}


shared_ptr<Contact>
ContactStorage::getContact(const Name& identity) const
{
  shared_ptr<Contact> contact;
  Profile profile;

  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT contact_alias, contact_keyName, contact_key, notBefore, notAfter, is_introducer FROM Contact where contact_namespace=?", -1, &stmt, 0);
  sqlite3_bind_text (stmt, 1, identity.toUri().c_str(), identity.toUri().size(), SQLITE_TRANSIENT);

  if(sqlite3_step (stmt) == SQLITE_ROW)
    {
      string alias(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0));
      string keyName(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)), sqlite3_column_bytes (stmt, 1));
      PublicKey key(sqlite3_column_text(stmt, 2), sqlite3_column_bytes (stmt, 2));
      time::system_clock::TimePoint notBefore = time::fromUnixTimestamp(time::milliseconds(sqlite3_column_int64 (stmt, 3)));
      time::system_clock::TimePoint notAfter = time::fromUnixTimestamp(time::milliseconds(sqlite3_column_int64 (stmt, 4)));
      int isIntroducer = sqlite3_column_int (stmt, 5);

      contact = shared_ptr<Contact>(new Contact(identity, alias, Name(keyName), notBefore, notAfter, key, isIntroducer));
    }
  sqlite3_finalize (stmt);

  sqlite3_prepare_v2 (m_db, "SELECT profile_type, profile_value FROM ContactProfile where profile_identity=?", -1, &stmt, 0);
  sqlite3_bind_text (stmt, 1, identity.toUri().c_str(), identity.toUri().size(), SQLITE_TRANSIENT);
  
  while(sqlite3_step (stmt) == SQLITE_ROW)
    {
      string type(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0));
      string value(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)), sqlite3_column_bytes (stmt, 1));
      profile[type] = value;
    }
  sqlite3_finalize (stmt);
  contact->setProfile(profile);

  if(contact->isIntroducer())
    {
      sqlite3_prepare_v2 (m_db, "SELECT trust_scope FROM TrustScope WHERE contact_namespace=?", -1, &stmt, 0);
      sqlite3_bind_text(stmt, 1, identity.toUri().c_str(), identity.toUri().size(), SQLITE_TRANSIENT);

      while(sqlite3_step (stmt) == SQLITE_ROW)
        {
          Name scope(string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0)));
          contact->addTrustScope(scope);
        }
      sqlite3_finalize (stmt);
    }

  return contact;
}


void
ContactStorage::updateIsIntroducer(const Name& identity, bool isIntroducer)
{
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "UPDATE Contact SET is_introducer=? WHERE contact_namespace=?", -1, &stmt, 0);
  sqlite3_bind_int(stmt, 1, (isIntroducer ? 1 : 0));
  sqlite3_bind_text(stmt, 2, identity.toUri().c_str(),  identity.toUri().size (), SQLITE_TRANSIENT);
  int res = sqlite3_step (stmt);
  sqlite3_finalize (stmt);
  return;
}

void 
ContactStorage::updateAlias(const Name& identity, string alias)
{
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "UPDATE Contact SET contact_alias=? WHERE contact_namespace=?", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, alias.c_str(), alias.size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, identity.toUri().c_str(),  identity.toUri().size (), SQLITE_TRANSIENT);
  int res = sqlite3_step (stmt);
  sqlite3_finalize (stmt);
  return;
}

bool
ContactStorage::doesContactExist(const Name& name)
{
  bool result = false;
  
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT count(*) FROM Contact WHERE contact_namespace=?", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, name.toUri().c_str(), name.toUri().size(), SQLITE_TRANSIENT);

  int res = sqlite3_step (stmt);
    
  if (res == SQLITE_ROW)
    {
      int countAll = sqlite3_column_int (stmt, 0);
      if (countAll > 0)
        result = true;
    } 
  sqlite3_finalize (stmt); 
  return result;
}

void
ContactStorage::getAllContacts(vector<shared_ptr<Contact> >& contacts) const
{
  vector<Name> contactNames;

  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT contact_namespace FROM Contact", -1, &stmt, 0);
  
  while(sqlite3_step (stmt) == SQLITE_ROW)
    {
      string identity(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes(stmt, 0));
      contactNames.push_back(Name(identity));
    }
  sqlite3_finalize (stmt);

  vector<Name>::iterator it  = contactNames.begin();
  vector<Name>::iterator end = contactNames.end();
  for(; it != end; it++)
    {
      shared_ptr<Contact> contact = getContact(*it);
      if(static_cast<bool>(contact))
        contacts.push_back(contact);
    }
}

void
ContactStorage::updateDnsData(const Block& data, 
                              const string& name, 
                              const string& type, 
                              const string& dataName)
{  
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "INSERT OR REPLACE INTO DnsData (dns_name, dns_type, dns_value, data_name) VALUES (?, ?, ?, ?)", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, name.c_str(), name.size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, type.c_str(), type.size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, reinterpret_cast<const char*>(data.wire()), data.size(), SQLITE_TRANSIENT); 
  sqlite3_bind_text(stmt, 4, dataName.c_str(), dataName.size(), SQLITE_TRANSIENT);
  int res = sqlite3_step(stmt);

  // _LOG_DEBUG("updateDnsData " << res);
  sqlite3_finalize(stmt);
}

shared_ptr<Data>
ContactStorage::getDnsData(const Name& dataName)
{
  shared_ptr<Data> data;

  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT dns_value FROM DnsData where data_name=?", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, dataName.toUri().c_str(), dataName.toUri().size(), SQLITE_TRANSIENT);
  
  if(sqlite3_step (stmt) == SQLITE_ROW)
    {
      data = make_shared<Data>();
      data->wireDecode(Block(reinterpret_cast<const uint8_t*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0)));
    }
  sqlite3_finalize(stmt);

  return data;
}

shared_ptr<Data>
ContactStorage::getDnsData(const string& name, const string& type)
{
  shared_ptr<Data> data;

  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT dns_value FROM DnsData where dns_name=? and dns_type=?", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, name.c_str(), name.size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, type.c_str(), type.size(), SQLITE_TRANSIENT);
  
  if(sqlite3_step (stmt) == SQLITE_ROW)
    {
      data = make_shared<Data>();
      data->wireDecode(Block(reinterpret_cast<const uint8_t*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0)));
    }
  sqlite3_finalize(stmt);

  return data;
}

}//chronos
