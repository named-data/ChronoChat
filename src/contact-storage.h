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

#include "contact.h"
#include "endorse-collection.pb.h"
#include <sqlite3.h>


namespace chronos{

class ContactStorage
{

public:
  struct Error : public std::runtime_error { Error(const std::string &what) : std::runtime_error(what) {} };

  ContactStorage(const ndn::Name& identity);
  
  ~ContactStorage() 
  {
    sqlite3_close(m_db); 
  }

  ndn::shared_ptr<Profile>
  getSelfProfile();
  
  void
  addSelfEndorseCertificate(const EndorseCertificate& endorseCertificate);

  void
  addEndorseCertificate(const EndorseCertificate& endorseCertificate, const ndn::Name& identity);

  void
  updateCollectEndorse(const EndorseCertificate& endorseCertificate);

  void
  getCollectEndorse(EndorseCollection& endorseCollection);

  void
  getEndorseList(const ndn::Name& identity, std::vector<std::string>& endorseList);



  void
  removeContact(const ndn::Name& identity);

  void
  addContact(const Contact& contact);

  ndn::shared_ptr<Contact>
  getContact(const ndn::Name& identity) const;

  void
  updateIsIntroducer(const ndn::Name& identity, bool isIntroducer);

  void 
  updateAlias(const ndn::Name& identity, std::string alias);

  void
  getAllContacts(std::vector<ndn::shared_ptr<Contact> >& contacts) const;

  void
  updateDnsSelfProfileData(const ndn::Data& data)
  {
    updateDnsData(data.wireEncode(), "N/A", "PROFILE", data.getName().toUri());
  }

  void
  updateDnsEndorseOthers(const ndn::Data& data, const std::string& endorsee)
  {
    updateDnsData(data.wireEncode(), endorsee, "ENDORSEE", data.getName().toUri());
  }
  
  void
  updateDnsOthersEndorse(const ndn::Data& data)
  {
    updateDnsData(data.wireEncode(), "N/A", "ENDORSED", data.getName().toUri()); 
  }

  ndn::shared_ptr<ndn::Data>
  getDnsData(const ndn::Name& name);

  ndn::shared_ptr<ndn::Data>
  getDnsData(const std::string& name, const std::string& type);

private:
  std::string
  getDBName();
  
  void
  initializeTable(const std::string& tableName, const std::string& sqlCreateStmt);

  bool
  doesContactExist(const ndn::Name& name);

  void
  updateDnsData(const ndn::Block& data, const std::string& name, const std::string& type, const std::string& dataName);

private:
  ndn::Name m_identity;

  sqlite3 *m_db;
};

}//chronos
#endif
