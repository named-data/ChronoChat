/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef LINKNDN_CONTACT_STORAGE_H
#define LINKNDN_CONTACT_STORAGE_H

#include <sqlite3.h>
#include "contact-item.h"
#include "endorse-certificate.h"
#include <ndn.cxx/security/identity/identity-manager.h>
#include <ndn.cxx/fields/signature-sha256-with-rsa.h>


class ContactStorage
{

public:
  ContactStorage();
  
  ~ContactStorage() 
  {sqlite3_close(m_db);}

  void
  setSelfProfileEntry(const ndn::Name& identity, const std::string& profileType, const ndn::Blob& profileValue);

  ndn::Ptr<Profile>
  getSelfProfile(const ndn::Name& identity);

  void
  addContact(const ContactItem& contactItem);

  void 
  updateAlias(const ndn::Name& identity, std::string alias);

  std::vector<ndn::Ptr<ContactItem> >
  getAllContacts() const;

  ndn::Ptr<ContactItem>
  getContact(const ndn::Name& name);
    
  ndn::Ptr<Profile>
  getSelfProfile(const ndn::Name& identity) const;

  ndn::Ptr<ndn::Blob>
  getSelfEndorseCertificate(const ndn::Name& identity);

  void
  updateSelfEndorseCertificate(ndn::Ptr<EndorseCertificate> endorseCertificate, const ndn::Name& identity);

  void
  addSelfEndorseCertificate(ndn::Ptr<EndorseCertificate> endorseCertificate, const ndn::Name& identity);
  
private:
  bool
  doesSelfEntryExist(const ndn::Name& identity, const std::string& profileType);

  bool
  doesContactExist(const ndn::Name& name);

private:
  sqlite3 *m_db;
};

#endif
