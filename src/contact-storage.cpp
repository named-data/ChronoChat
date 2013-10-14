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

#include <string>
#include <boost/filesystem.hpp>

using namespace std;
using namespace ndn;
namespace fs = boost::filesystem;


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

  // Check if TrustedContact table exists
  sqlite3_stmt *stmt;
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
      sqlite3_finalize (stmt);
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
  sqlite3_finalize (stmt);
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
