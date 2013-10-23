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
#include "exception.h"

#include <boost/filesystem.hpp>
#include <ndn.cxx/fields/signature-sha256-with-rsa.h>
#include "logging.h"

using namespace std;
using namespace ndn;
namespace fs = boost::filesystem;

INIT_LOGGER ("ContactStorage");

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

const string INIT_TC_TABLE = "\
CREATE TABLE IF NOT EXISTS                                           \n \
  TrustedContact(                                                    \n \
      contact_namespace BLOB NOT NULL,                               \n \
      contact_alias     BLOB NOT NULL,                               \n \
      self_certificate  BLOB NOT NULL,                               \n \
      trust_scope       BLOB NOT NULL,                               \n \
                                                                     \
      PRIMARY KEY (contact_namespace)                                \n \
  );                                                                 \n \
                                                                     \
CREATE INDEX tc_index ON TrustedContact(contact_namespace);          \n \
";

const string INIT_NC_TABLE = "\
CREATE TABLE IF NOT EXISTS                                           \n \
  NormalContact(                                                     \n \
      contact_namespace BLOB NOT NULL,                               \n \
      contact_alias     BLOB NOT NULL,                               \n \
      self_certificate  BLOB NOT NULL,                               \n \
                                                                     \
      PRIMARY KEY (contact_namespace)                                \n \
  );                                                                 \n \
                                                                     \
CREATE INDEX nc_index ON NormalContact(contact_namespace);           \n \
";

ContactStorage::ContactStorage()
{
  fs::path chronosDir = fs::path(getenv("HOME")) / ".chronos";
  fs::create_directories (chronosDir);

  int res = sqlite3_open((chronosDir / "chronos.db").c_str (), &m_db);
  if (res != SQLITE_OK)
    throw LnException("Chronos DB cannot be open/created");

  // Check if SelfProfile table exists
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT name FROM sqlite_master WHERE type='table' And name='SelfProfile'", -1, &stmt, 0);
  res = sqlite3_step (stmt);

  bool spTableExist = false;
  if (res == SQLITE_ROW)
      spTableExist = true;
  sqlite3_finalize (stmt);

  if(!spTableExist)
    {
      char *errmsg = 0;
      res = sqlite3_exec (m_db, INIT_SP_TABLE.c_str (), NULL, NULL, &errmsg);
      if (res != SQLITE_OK && errmsg != 0)
        throw LnException("Init \"error\" in SelfProfile");
    }

  // Check if SelfEndorse table exists
  sqlite3_prepare_v2 (m_db, "SELECT name FROM sqlite_master WHERE type='table' And name='SelfEndorse'", -1, &stmt, 0);
  res = sqlite3_step (stmt);

  bool seTableExist = false;
  if (res == SQLITE_ROW)
      seTableExist = true;
  sqlite3_finalize (stmt);

  if(!seTableExist)
    {
      char *errmsg = 0;
      res = sqlite3_exec (m_db, INIT_SE_TABLE.c_str (), NULL, NULL, &errmsg);
      if (res != SQLITE_OK && errmsg != 0)
        throw LnException("Init \"error\" in SelfEndorse");
    }


  // Check if TrustedContact table exists
  sqlite3_prepare_v2 (m_db, "SELECT name FROM sqlite_master WHERE type='table' And name='TrustedContact'", -1, &stmt, 0);
  res = sqlite3_step (stmt);

  bool tcTableExist = false;
  if (res == SQLITE_ROW)
      tcTableExist = true;
  sqlite3_finalize (stmt);

  if(!tcTableExist)
    {
      char *errmsg = 0;
      res = sqlite3_exec (m_db, INIT_TC_TABLE.c_str (), NULL, NULL, &errmsg);
      if (res != SQLITE_OK && errmsg != 0)
        throw LnException("Init \"error\" in TrustedContact");
    }
    
  // Check if NormalContact table exists
  sqlite3_prepare_v2 (m_db, "SELECT name FROM sqlite_master WHERE type='table' And name='NormalContact'", -1, &stmt, 0);
  res = sqlite3_step (stmt);

  bool ncTableExist = false;
  if (res == SQLITE_ROW)
      ncTableExist = true;
  sqlite3_finalize (stmt);

  if(!ncTableExist)
    {
      char *errmsg = 0;
      res = sqlite3_exec (m_db, INIT_NC_TABLE.c_str (), NULL, NULL, &errmsg);
        
      if (res != SQLITE_OK && errmsg != 0)
        throw LnException("Init \"error\" in NormalContact");
    }
}

bool
ContactStorage::doesSelfEntryExist(const Name& identity, const string& profileType)
{
  bool result = false;
  
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT count(*) FROM SelfProfile WHERE profile_type=? and profile_identity=?", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, profileType.c_str(), profileType.size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, identity.toUri().c_str(), identity.toUri().size(), SQLITE_TRANSIENT);

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
ContactStorage::setSelfProfileEntry(const Name& identity, const string& profileType, const Blob& profileValue)
{
  sqlite3_stmt *stmt;  
  if(doesSelfEntryExist(identity, profileType))
    {
      sqlite3_prepare_v2 (m_db, "UPDATE SelfProfile SET profile_value=? WHERE profile_type=? and profile_identity=?", -1, &stmt, 0);
      sqlite3_bind_text(stmt, 1, profileValue.buf(), profileValue.size(), SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 2, profileType.c_str(), profileType.size(), SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 3, identity.toUri().c_str(), identity.toUri().size(), SQLITE_TRANSIENT);
    }
  else
    {
      sqlite3_prepare_v2 (m_db, "INSERT INTO SelfProfile (profile_identity, profile_type, profile_value) values (?, ?, ?)", -1, &stmt, 0);
      sqlite3_bind_text(stmt, 1, identity.toUri().c_str(), identity.toUri().size(), SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 2, profileType.c_str(), profileType.size(), SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 3, profileValue.buf(), profileValue.size(), SQLITE_TRANSIENT);
    }
  sqlite3_step (stmt);
  sqlite3_finalize (stmt);
}

Ptr<Profile>
ContactStorage::getSelfProfile(const Name& identity)
{
  _LOG_DEBUG("getSelfProfile " << identity.toUri());
  sqlite3_stmt *stmt;
  Ptr<Profile> profile = Ptr<Profile>(new Profile(identity));
  
  sqlite3_prepare_v2(m_db, "SELECT profile_type, profile_value FROM SelfProfile WHERE profile_identity=?", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, identity.toUri().c_str(), identity.toUri().size(), SQLITE_TRANSIENT);

  while(sqlite3_step (stmt) == SQLITE_ROW)
    {
      _LOG_DEBUG("entry");
      string profileType(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0));
      Blob profileValue(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)), sqlite3_column_bytes (stmt, 1));

      profile->setProfileEntry(profileType, profileValue );
    }

  return profile;
}

void
ContactStorage::addTrustedContact(const TrustedContact& trustedContact)
{
  if(doesTrustedContactExist(trustedContact.getNameSpace()))
    throw LnException("Trusted Contact has already existed");
  
  sqlite3_stmt *stmt;  
  sqlite3_prepare_v2 (m_db, 
                      "INSERT INTO TrustedContact (contact_namespace, contact_alias, self_certificate, trust_scope) values (?, ?, ?, ?)", 
                      -1, 
                      &stmt, 
                      0);
  
  sqlite3_bind_text(stmt, 1, trustedContact.getNameSpace().toUri().c_str(),  trustedContact.getNameSpace().toUri().size (), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, trustedContact.getAlias().c_str(), trustedContact.getAlias().size(), SQLITE_TRANSIENT);
  Ptr<Blob> selfCertificateBlob = trustedContact.getSelfEndorseCertificate().encodeToWire();
  sqlite3_bind_text(stmt, 3, selfCertificateBlob->buf(), selfCertificateBlob->size(), SQLITE_TRANSIENT);
  Ptr<Blob> trustScopeBlob = trustedContact.getTrustScopeBlob();
  sqlite3_bind_text(stmt, 4, trustScopeBlob->buf(), trustScopeBlob->size(),SQLITE_TRANSIENT);

  int res = sqlite3_step (stmt);
  sqlite3_finalize (stmt);
}
  
void
ContactStorage::addNormalContact(const ContactItem& normalContact)
{
  if(doesNormalContactExist(normalContact.getNameSpace()))
    throw LnException("Normal Contact has already existed");

  sqlite3_stmt *stmt;  
  sqlite3_prepare_v2 (m_db, 
                      "INSERT INTO NormalContact (contact_namespace, contact_alias, self_certificate) values (?, ?, ?)", 
                      -1, 
                      &stmt, 
                      0);

  sqlite3_bind_text(stmt, 1, normalContact.getNameSpace().toUri().c_str(),  normalContact.getNameSpace().toUri().size (), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, normalContact.getAlias().c_str(), normalContact.getAlias().size(), SQLITE_TRANSIENT);
  Ptr<Blob> selfCertificateBlob = normalContact.getSelfEndorseCertificate().encodeToWire();
  sqlite3_bind_text(stmt, 3, selfCertificateBlob->buf(), selfCertificateBlob->size(), SQLITE_TRANSIENT);

  int res = sqlite3_step (stmt);
  // _LOG_DEBUG("res " << res);
  sqlite3_finalize (stmt);
}

void 
ContactStorage::updateAlias(const ndn::Name& identity, std::string alias)
{
  if(doesNormalContactExist(identity))
    {
      sqlite3_stmt *stmt;
      sqlite3_prepare_v2 (m_db, "UPDATE NormalContact SET contact_alias=? WHERE contact_namespace=?", -1, &stmt, 0);
      sqlite3_bind_text(stmt, 1, alias.c_str(), alias.size(), SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 2, identity.toUri().c_str(),  identity.toUri().size (), SQLITE_TRANSIENT);
      int res = sqlite3_step (stmt);
      sqlite3_finalize (stmt);
      return;
    }
  if(doesTrustedContactExist(identity))
    {
      sqlite3_stmt *stmt;
      sqlite3_prepare_v2 (m_db, "UPDATE TrustedContact SET contact_alias=? WHERE contact_namespace=?", -1, &stmt, 0);
      sqlite3_bind_text(stmt, 1, alias.c_str(), alias.size(), SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 2, identity.toUri().c_str(),  identity.toUri().size (), SQLITE_TRANSIENT);
      int res = sqlite3_step (stmt);
      sqlite3_finalize (stmt);
      return;
    }
}

bool
ContactStorage::doesContactExist(const Name& name, bool normal)
{
  bool result = false;
  
  sqlite3_stmt *stmt;
  if(normal)
    sqlite3_prepare_v2 (m_db, "SELECT count(*) FROM NormalContact WHERE contact_namespace=?", -1, &stmt, 0);
  else
    sqlite3_prepare_v2 (m_db, "SELECT count(*) FROM TrustedContact WHERE contact_namespace=?", -1, &stmt, 0);
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

vector<Ptr<TrustedContact> >
ContactStorage::getAllTrustedContacts() const
{
  vector<Ptr<TrustedContact> > trustedContacts;

  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, 
                      "SELECT contact_alias, self_certificate, trust_scope FROM TrustedContact", 
                      -1, 
                      &stmt, 
                      0);
  
  while( sqlite3_step (stmt) == SQLITE_ROW)
    {
      string alias(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0));
      Ptr<Blob> certBlob = Ptr<Blob>(new Blob(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)), sqlite3_column_bytes (stmt, 1)));
      Ptr<Data> certData = Data::decodeFromWire(certBlob);
      EndorseCertificate endorseCertificate(*certData);
      string trustScope(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)), sqlite3_column_bytes (stmt, 2));

      trustedContacts.push_back(Ptr<TrustedContact>(new TrustedContact(endorseCertificate, trustScope, alias)));      
    }

  return trustedContacts;
}

vector<Ptr<ContactItem> >
ContactStorage::getAllNormalContacts() const
{
  vector<Ptr<ContactItem> > normalContacts;

  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, 
                      "SELECT contact_alias, self_certificate FROM NormalContact", 
                      -1, 
                      &stmt, 
                      0);
  
  while( sqlite3_step (stmt) == SQLITE_ROW)
    {
      string alias(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0));
      Ptr<Blob> certBlob = Ptr<Blob>(new Blob(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)), sqlite3_column_bytes (stmt, 1)));
      Ptr<Data> certData = Data::decodeFromWire(certBlob);
      EndorseCertificate endorseCertificate(*certData);

      normalContacts.push_back(Ptr<ContactItem>(new ContactItem(endorseCertificate, alias)));      
    }
  
  return normalContacts;
}

Ptr<ContactItem>
ContactStorage::getNormalContact(const Name& name)
{
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, 
                      "SELECT contact_alias, self_certificate FROM NormalContact where contact_namespace=?", 
                      -1, 
                      &stmt, 
                      0);
  sqlite3_bind_text (stmt, 1, name.toUri().c_str(), name.toUri().size(), SQLITE_TRANSIENT);
  
  if( sqlite3_step (stmt) == SQLITE_ROW)
    {
      string alias(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0));
      Ptr<Blob> certBlob = Ptr<Blob>(new Blob(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)), sqlite3_column_bytes (stmt, 1)));
      Ptr<Data> certData = Data::decodeFromWire(certBlob);
      EndorseCertificate endorseCertificate(*certData);

      return Ptr<ContactItem>(new ContactItem(endorseCertificate, alias));      
    } 
  return NULL;
}

Ptr<TrustedContact>
ContactStorage::getTrustedContact(const Name& name)
{
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, 
                      "SELECT contact_alias, self_certificate, trust_scope FROM TrustedContact where contact_namespace=?", 
                      -1, 
                      &stmt, 
                      0);
  sqlite3_bind_text (stmt, 1, name.toUri().c_str(), name.toUri().size(), SQLITE_TRANSIENT);
  
  if( sqlite3_step (stmt) == SQLITE_ROW)
    {
      string alias(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0));
      Ptr<Blob> certBlob = Ptr<Blob>(new Blob(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)), sqlite3_column_bytes (stmt, 1)));
      Ptr<Data> certData = Data::decodeFromWire(certBlob);
      EndorseCertificate endorseCertificate(*certData);
      string trustScope(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)), sqlite3_column_bytes (stmt, 2));

      return Ptr<TrustedContact>(new TrustedContact(endorseCertificate, trustScope, alias));      
    }
  return NULL;
}

Ptr<Profile>
ContactStorage::getSelfProfile(const Name& identity) const
{  
  Ptr<Profile> profile = Ptr<Profile>(new Profile(identity));
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT profile_type, profile_value FROM SelfProfile WHERE profile_identity=?", -1, &stmt, 0);
  sqlite3_bind_text (stmt, 1, identity.toUri().c_str(), identity.toUri().size(), SQLITE_TRANSIENT);

  while( sqlite3_step (stmt) == SQLITE_ROW)
    {
      string profileType(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0));
      Blob profileValue(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)), sqlite3_column_bytes (stmt, 1));
      profile->setProfileEntry(profileType, profileValue);
    }

  return profile;
}

Ptr<Blob>
ContactStorage::getSelfEndorseCertificate(const Name& identity)
{
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "SELECT endorse_data FROM SelfEndorse where identity=?", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, identity.toUri().c_str(), identity.toUri().size(), SQLITE_TRANSIENT);

  Ptr<Blob> result = NULL;
  if(sqlite3_step (stmt) == SQLITE_ROW)
    result = Ptr<Blob>(new Blob(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes (stmt, 0)));

  sqlite3_finalize (stmt);

  return result;
}

void
ContactStorage::updateSelfEndorseCertificate(Ptr<EndorseCertificate> newEndorseCertificate, const Name& identity)
{
  Ptr<Blob> newEndorseCertificateBlob = newEndorseCertificate->encodeToWire();

  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "UPDATE SelfEndorse SET endorse_data=? WHERE identity=?", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, newEndorseCertificateBlob->buf(), newEndorseCertificateBlob->size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, identity.toUri().c_str(), identity.toUri().size(), SQLITE_TRANSIENT);
  sqlite3_step(stmt);
}

void
ContactStorage::addSelfEndorseCertificate(Ptr<EndorseCertificate> newEndorseCertificate, const Name& identity)
{
  Ptr<Blob> newEndorseCertificateBlob = newEndorseCertificate->encodeToWire();

  sqlite3_stmt *stmt;
  sqlite3_prepare_v2 (m_db, "INSERT INTO SelfEndorse (identity, endorse_data) values (?, ?)", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, identity.toUri().c_str(), identity.toUri().size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, newEndorseCertificateBlob->buf(), newEndorseCertificateBlob->size(), SQLITE_TRANSIENT);
  sqlite3_step(stmt);
}
