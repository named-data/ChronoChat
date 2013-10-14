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

class ContactStorage
{
public:
  ContactStorage();
  
  ~ContactStorage() {}

  void
  addTrustedContact(const TrustedContact& trustedContact);
  
  void
  addNormalContact(const ContactItem& contactItem);

  std::vector<ndn::Ptr<TrustedContact> >
  getAllTrustedContacts() const;

  std::vector<ndn::Ptr<ContactItem> >
  getAllNormalContacts() const;

private:
  inline bool
  doesTrustedContactExist(const ndn::Name& name)
  { return doesContactExist(name, false); }

  inline bool
  doesNormalContactExist(const ndn::Name& name)
  { return doesContactExist(name, true); }

  bool
  doesContactExist(const ndn::Name& name, bool normal);
  
private:
  sqlite3 *m_db;
};

#endif
