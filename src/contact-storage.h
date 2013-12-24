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
#include <ndn-cpp/security/identity/identity-manager.hpp>


class ContactStorage
{

public:
  ContactStorage();
  
  ~ContactStorage() 
  {sqlite3_close(m_db);}

  void
  setSelfProfileEntry(const ndn::Name& identity, const std::string& profileType, const ndn::Blob& profileValue);

  ndn::ptr_lib::shared_ptr<Profile>
  getSelfProfile(const ndn::Name& identity);

  void
  removeContact(const ndn::Name& identity);

  void
  addContact(const ContactItem& contactItem);

  void
  updateIsIntroducer(const ndn::Name& identity, bool isIntroducer);

  void 
  updateAlias(const ndn::Name& identity, std::string alias);

  void
  getAllContacts(std::vector<ndn::ptr_lib::shared_ptr<ContactItem> >& contacts) const;

  ndn::ptr_lib::shared_ptr<ContactItem>
  getContact(const ndn::Name& name);
    
  ndn::ptr_lib::shared_ptr<Profile>
  getSelfProfile(const ndn::Name& identity) const;


  //SelfEndorse
  ndn::Blob
  getSelfEndorseCertificate(const ndn::Name& identity);

  void
  updateSelfEndorseCertificate(const EndorseCertificate& endorseCertificate, const ndn::Name& identity);

  void
  addSelfEndorseCertificate(const EndorseCertificate& endorseCertificate, const ndn::Name& identity);


  //ProfileEndorse
  ndn::Blob
  getEndorseCertificate(const ndn::Name& identity);

  void
  updateEndorseCertificate(const EndorseCertificate& endorseCertificate, const ndn::Name& identity);

  void
  addEndorseCertificate(const EndorseCertificate& endorseCertificate, const ndn::Name& identity);

  void
  getEndorseList(const ndn::Name& identity, std::vector<std::string>& endorseList);

  
  //CollectEndorse
  void
  updateCollectEndorse(const EndorseCertificate& endorseCertificate);

  void
  getCollectEndorseList(const ndn::Name& name, std::vector<ndn::Blob>& endorseList);
  

private:
  bool
  doesSelfEntryExist(const ndn::Name& identity, const std::string& profileType);

  bool
  doesContactExist(const ndn::Name& name);

private:
  sqlite3 *m_db;
};

#endif
