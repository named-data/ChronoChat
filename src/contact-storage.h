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
#include "trusted-contact.h"
#include "contact-item.h"
#include "endorse-certificate.h"
#include <ndn.cxx/security/identity/identity-manager.h>
#include <ndn.cxx/fields/signature-sha256-with-rsa.h>

class ContactStorage
{
public:
  ContactStorage(ndn::Ptr<ndn::security::IdentityManager> identityManager);
  
  ~ContactStorage() {}

  void
  setSelfProfileEntry(const ndn::Name& identity, const std::string& profileType, const ndn::Blob& profileValue);

  ndn::Ptr<Profile>
  getSelfProfile(const ndn::Name& identity);

  void
  addTrustedContact(const TrustedContact& trustedContact);
  
  void
  addNormalContact(const ContactItem& contactItem);

  std::vector<ndn::Ptr<TrustedContact> >
  getAllTrustedContacts() const;

  std::vector<ndn::Ptr<ContactItem> >
  getAllNormalContacts() const;

  void
  updateProfileData(const ndn::Name& identity) const;
  
  inline ndn::Ptr<ndn::security::IdentityManager> 
  getIdentityManager()
  { return m_identityManager; }
  
private:
  bool
  doesSelfEntryExist(const ndn::Name& identity, const std::string& profileType);

  inline bool
  doesTrustedContactExist(const ndn::Name& name)
  { return doesContactExist(name, false); }

  inline bool
  doesNormalContactExist(const ndn::Name& name)
  { return doesContactExist(name, true); }

  bool
  doesContactExist(const ndn::Name& name, bool normal);

  ndn::Ptr<Profile>
  getSelfProfile(const ndn::Name& identity) const;

  ndn::Ptr<EndorseCertificate>
  getSignedSelfEndorseCertificate(const ndn::Name& identity,
                                  const Profile& profile) const;
  
private:
  ndn::Ptr<ndn::security::IdentityManager> m_identityManager;
  sqlite3 *m_db;
};

#endif
