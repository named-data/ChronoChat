/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOS_CONTACT_STORAGE_H
#define CHRONOS_CONTACT_STORAGE_H

#include "contact-item.h"
#include <sqlite3.h>


namespace chronos{

class ContactStorage
{

public:
  struct Error : public std::runtime_error { Error(const std::string &what) : std::runtime_error(what) {} };

  ContactStorage();
  
  ~ContactStorage() 
  { sqlite3_close(m_db); }

  ndn::shared_ptr<Profile>
  getSelfProfile(const ndn::Name& identity) const;

  // ndn::Block
  // getSelfEndorseCertificate(const ndn::Name& identity);
  
  void
  addSelfEndorseCertificate(const EndorseCertificate& endorseCertificate, const ndn::Name& identity);

  // ndn::Block
  // getEndorseCertificate(const ndn::Name& identity);

  void
  addEndorseCertificate(const EndorseCertificate& endorseCertificate, const ndn::Name& identity);

  void
  updateCollectEndorse(const EndorseCertificate& endorseCertificate);

  void
  getCollectEndorseList(const ndn::Name& name, std::vector<ndn::Buffer>& endorseList);

  void
  getEndorseList(const ndn::Name& identity, std::vector<std::string>& endorseList);



  void
  removeContact(const ndn::Name& identity);

  void
  addContact(const ContactItem& contactItem);

  ndn::shared_ptr<ContactItem>
  getContact(const ndn::Name& name);

  void
  updateIsIntroducer(const ndn::Name& identity, bool isIntroducer);

  void 
  updateAlias(const ndn::Name& identity, std::string alias);

  void
  getAllContacts(std::vector<ndn::shared_ptr<ContactItem> >& contacts) const;

 
  

private:
  void
  initializeTable(const std::string& tableName, const std::string& sqlCreateStmt);

  bool
  doesContactExist(const ndn::Name& name);

private:
  sqlite3 *m_db;
};

}//chronos
#endif
