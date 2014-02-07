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
#include "logging.h"

using namespace std;
using namespace ndn;
namespace fs = boost::filesystem;

INIT_LOGGER ("ContactStorage");

namespace chronos{

const string INIT_SP_TABLE = "\
CREATE TABLE IF NOT EXISTS                                           \n \
  SelfProfile(                                                       \n \
      profile_identity  BLOB NOT NULL,                               \n \
      profile_type      BLOB NOT NULL,                               \n \
      profile_value     BLOB NOT NULL,                               \n \
                                                                     \
      PRIMARY KEY (profile_identity, profile_type)                   \n \
  );                                                                 \n \
                                                                     \
CREATE INDEX sp_index ON SelfProfile(profile_identity,profile_type); \n \
";

const string INIT_SE_TABLE = "\
CREATE TABLE IF NOT EXISTS                                           \n \
  SelfEndorse(                                                       \n \
      identity          BLOB NOT NULL UNIQUE,                        \n \
      endorse_data      BLOB NOT NULL,                               \n \
                                                                     \
      PRIMARY KEY (identity)                                         \n \
  );                                                                 \n \
CREATE INDEX se_index ON SelfEndorse(identity);                      \n \
";

const string INIT_CONTACT_TABLE = "\
CREATE TABLE IF NOT EXISTS                                           \n \
  Contact(                                                           \n \
      contact_namespace BLOB NOT NULL,                               \n \
      contact_alias     BLOB NOT NULL,                               \n \
      self_certificate  BLOB NOT NULL,                               \n \
      is_introducer     INTEGER DEFAULT 0,                           \n \
                                                                     \
      PRIMARY KEY (contact_namespace)                                \n \
  );                                                                 \n \
                                                                     \
CREATE INDEX contact_index ON Contact(contact_namespace);            \n \
";

const string INIT_TS_TABLE = "\
CREATE TABLE IF NOT EXISTS                                           \n \
  TrustScope(                                                        \n \
      id                INTEGER PRIMARY KEY AUTOINCREMENT,           \n \
      contact_namespace BLOB NOT NULL,                               \n \
      trust_scope       BLOB NOT NULL                                \n \
  );                                                                 \n \
                                                                     \
CREATE INDEX ts_index ON TrustScope(contact_namespace);              \n \
";

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
";

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
";

const string INIT_CE_TABLE = "\
CREATE TABLE IF NOT EXISTS                                           \n \
  CollectEndorse(                                                    \n \
      endorsee          BLOB NOT NULL,                               \n \
      endorser          BLOB NOT NULL,                               \n \
      endorse_name      BLOB NOT NULL,                               \n \
      endorse_data      BLOB NOT NULL,                               \n \
                                                                     \
      PRIMARY KEY (endorsee, endorser)                               \n \
  );                                                                 \n \
                                                                     \
CREATE INDEX ce_index ON CollectEndorse(endorsee);                   \n \
";

ContactStorage::ContactStorage()
{
  fs::path chronosDir = fs::path(getenv("HOME")) / ".chronos";
  fs::create_directories (chronosDir);

  int res = sqlite3_open((chronosDir / "chronos.db").c_str (), &m_db);
  if (res != SQLITE_OK)
    throw Error("Chronos DB cannot be open/created");

  initializeTable("SelfProfile", INIT_SP_TABLE);
  initializeTable("SelfEndorse", INIT_SE_TABLE);
  initializeTable("Contact", INIT_CONTACT_TABLE);
  initializeTable("TrustScope", INIT_TS_TABLE);
  initializeTable("ContactProfile", INIT_CP_TABLE);
  initializeTable("ProfileEndorse", INIT_PE_TABLE);
  initializeTable("CollectEndorse", INIT_CE_TABLE);

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
ContactStorage::getSelfProfile(const Name& identity) const
{  
  shared_ptr<Profile> profile;
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT profile_type, profile_value FROM SelfProfile WHERE profile_identity=?", -1, &stmt, 0);
  sqlite3_bind_text (stmt, 1, identity.toUri().c_str(), identity.toUri().size(), SQLITE_TRANSIENT);

  while( sqlite3_step (stmt) == SQLITE_ROW)
    {
      profile = make_shared<Profile>(identity);
      string profileType(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0));
      string profileValue(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)), sqlite3_column_bytes (stmt, 1));
      (*profile)[profileType] = profileValue;
    }

  sqlite3_finalize(stmt);

  return profile;
}

// Block
// ContactStorage::getSelfEndorseCertificate(const Name& identity)
// {
//   sqlite3_stmt *stmt;
//   sqlite3_prepare_v2 (m_db, "SELECT endorse_data FROM SelfEndorse where identity=?", -1, &stmt, 0);
//   sqlite3_bind_text(stmt, 1, identity.toUri().c_str(), identity.toUri().size(), SQLITE_TRANSIENT);

//   if(sqlite3_step (stmt) == SQLITE_ROW)
//     {
//       Block result(reinterpret_cast<const uint8_t*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0));
//       sqlite3_finalize (stmt);
//       return result;
//     }

//   sqlite3_finalize (stmt);

//   throw Error("ContactStorage: No self-endorse certificate found!");
// }

void
ContactStorage::addSelfEndorseCertificate(const EndorseCertificate& newEndorseCertificate, const Name& identity)
{
  const Block& newEndorseCertificateBlock = newEndorseCertificate.wireEncode();

  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "INSERT OR REPLACE INTO SelfEndorse (identity, endorse_data) values (?, ?)", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, identity.toUri().c_str(), identity.toUri().size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, reinterpret_cast<const char*>(newEndorseCertificateBlock.wire()), newEndorseCertificateBlock.size(), SQLITE_TRANSIENT);
  sqlite3_step(stmt);

  sqlite3_finalize (stmt);
}

// Block
// ContactStorage::getEndorseCertificate(const Name& identity)
// {
//   sqlite3_stmt *stmt;
//   sqlite3_prepare_v2 (m_db, "SELECT endorse_data FROM ProfileEndorse where identity=?", -1, &stmt, 0);
//   sqlite3_bind_text(stmt, 1, identity.toUri().c_str(), identity.toUri().size(), SQLITE_TRANSIENT);

  
//   if(sqlite3_step (stmt) == SQLITE_ROW)
//     {
//       Block result(reinterpret_cast<const uint8_t*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0));
//       sqlite3_finalize (stmt);
//       return result;
//     }

//   sqlite3_finalize (stmt);

//   throw Error("ContactStorage: No endorse certificate found!");
  
//   return Block();
// }

void
ContactStorage::addEndorseCertificate(const EndorseCertificate& endorseCertificate, const Name& identity)
{
  const Block& newEndorseCertificateBlock = endorseCertificate.wireEncode();

  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "INSERT OR REPLACE INTO ProfileEndorse (identity, endorse_data) values (?, ?)", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, identity.toUri().c_str(), identity.toUri().size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, reinterpret_cast<const char*>(newEndorseCertificateBlock.value()), newEndorseCertificateBlock.size(), SQLITE_TRANSIENT);
  sqlite3_step(stmt);

  sqlite3_finalize (stmt);
}

void
ContactStorage::updateCollectEndorse(const EndorseCertificate& endorseCertificate)
{
  Name endorserName = endorseCertificate.getSigner();
  Name keyName = endorseCertificate.getPublicKeyName();
  Name endorseeName = keyName.getPrefix(keyName.size()-1);
  Name getCertName = endorseCertificate.getName();

  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "INSERT OR REPLACE INTO CollectEndorse (endorser, endorsee, endorse_name, endorse_data) VALUES (?, ?, ?, ?)", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, endorserName.toUri().c_str(), endorserName.toUri().size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, endorseeName.toUri().c_str(), endorseeName.toUri().size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, getCertName.toUri().c_str(), getCertName.toUri().size(), SQLITE_TRANSIENT);
  const Block &block = endorseCertificate.wireEncode();
  sqlite3_bind_text(stmt, 4, reinterpret_cast<const char*>(block.wire()), block.size(), SQLITE_TRANSIENT);
  int res = sqlite3_step (stmt);
  sqlite3_finalize (stmt); 
  return;
}

void
ContactStorage::getCollectEndorseList(const Name& name, vector<Buffer>& endorseList)
{
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT endorse_data FROM CollectEndorse WHERE endorsee=?", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, name.toUri().c_str(), name.toUri().size(), SQLITE_TRANSIENT);

  while(sqlite3_step (stmt) == SQLITE_ROW)
    {
      Buffer blob(reinterpret_cast<const uint8_t*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0));
      endorseList.push_back(blob);
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
ContactStorage::removeContact(const Name& contactNameSpace)
{
  string identity = contactNameSpace.toUri();
  
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
ContactStorage::addContact(const ContactItem& contact)
{
  if(doesContactExist(contact.getNameSpace()))
    throw Error("Normal Contact has already existed");

  bool isIntroducer = contact.isIntroducer();

  sqlite3_stmt *stmt;  
  sqlite3_prepare_v2 (m_db, 
                      "INSERT INTO Contact (contact_namespace, contact_alias, self_certificate, is_introducer) values (?, ?, ?, ?)", 
                      -1, 
                      &stmt, 
                      0);

  sqlite3_bind_text(stmt, 1, contact.getNameSpace().toUri().c_str(),  contact.getNameSpace().toUri().size (), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, contact.getAlias().c_str(), contact.getAlias().size(), SQLITE_TRANSIENT);
  const Block& selfCertificateBlock = contact.getSelfEndorseCertificate().wireEncode();
  sqlite3_bind_text(stmt, 3, reinterpret_cast<const char*>(selfCertificateBlock.wire()), selfCertificateBlock.size(), SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 4, (isIntroducer ? 1 : 0));

  int res = sqlite3_step (stmt);
  sqlite3_finalize (stmt);

  const Profile&  profile = contact.getSelfEndorseCertificate().getProfile();
  Profile::const_iterator it = profile.begin();
  string identity = contact.getNameSpace().toUri();
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
      ContactItem::const_iterator it = contact.trustScopeBegin();
      string nameSpace = contact.getNameSpace().toUri();
      
      while(it != contact.trustScopeEnd())
        {
          sqlite3_prepare_v2 (m_db, 
                              "INSERT INTO TrustScope (contact_namespace, trust_scope) values (?, ?)", 
                              -1, 
                              &stmt, 
                              0);
          sqlite3_bind_text(stmt, 1, nameSpace.c_str(),  nameSpace.size (), SQLITE_TRANSIENT);
          sqlite3_bind_text(stmt, 2, it->first.toUri().c_str(), it->first.toUri().size(), SQLITE_TRANSIENT);
          res = sqlite3_step (stmt);
          sqlite3_finalize (stmt);          
          it++;
        }
    }
}


shared_ptr<ContactItem>
ContactStorage::getContact(const Name& name)
{
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT contact_alias, self_certificate, is_introducer FROM Contact where contact_namespace=?", -1, &stmt, 0);
  sqlite3_bind_text (stmt, 1, name.toUri().c_str(), name.toUri().size(), SQLITE_TRANSIENT);
  
  if( sqlite3_step (stmt) == SQLITE_ROW)
    {
      string alias(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0));

      Data certData;
      certData.wireDecode(Block(reinterpret_cast<const uint8_t*>(sqlite3_column_text(stmt, 1)), sqlite3_column_bytes (stmt, 1)));
      EndorseCertificate endorseCertificate(certData);

      int isIntroducer = sqlite3_column_int (stmt, 2);

      sqlite3_finalize (stmt);
      
      shared_ptr<ContactItem> contact = make_shared<ContactItem>(endorseCertificate, isIntroducer, alias);

      if(contact->isIntroducer())
        {
          sqlite3_prepare_v2 (m_db, "SELECT trust_scope FROM TrustScope WHERE contact_namespace=?", -1, &stmt, 0);
          sqlite3_bind_text(stmt, 1, name.toUri().c_str(), name.toUri().size(), SQLITE_TRANSIENT);

          while( sqlite3_step (stmt) == SQLITE_ROW)
            {
              Name scope(string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0)));
              contact->addTrustScope(scope);
            }
          sqlite3_finalize (stmt);  
        }

      return contact;      
    } 
  return shared_ptr<ContactItem>();
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
ContactStorage::getAllContacts(vector<shared_ptr<ContactItem> >& contacts) const
{
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT contact_alias, self_certificate, is_introducer FROM Contact", -1, &stmt, 0);
  
  while( sqlite3_step (stmt) == SQLITE_ROW)
    {
      string alias(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0));

      Data certData;
      certData.wireDecode(Block(reinterpret_cast<const uint8_t*>(sqlite3_column_text(stmt, 1)), sqlite3_column_bytes (stmt, 1)));
      EndorseCertificate endorseCertificate(certData);

      int isIntroducer = sqlite3_column_int (stmt, 2);

      contacts.push_back(make_shared<ContactItem>(endorseCertificate, isIntroducer, alias));      
    }
  sqlite3_finalize (stmt);  

  vector<shared_ptr<ContactItem> >::iterator it = contacts.begin();
  for(; it != contacts.end(); it++)
    {
      if((*it)->isIntroducer())
        {
          sqlite3_prepare_v2 (m_db, "SELECT trust_scope FROM TrustScope WHERE contact_namespace=?", -1, &stmt, 0);
          sqlite3_bind_text(stmt, 1, (*it)->getNameSpace().toUri().c_str(), (*it)->getNameSpace().toUri().size(), SQLITE_TRANSIENT);

          while( sqlite3_step (stmt) == SQLITE_ROW)
            {
              Name scope(string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0)));
              (*it)->addTrustScope(scope);
            }
          sqlite3_finalize (stmt);  
        }
    }
}

}//chronos
