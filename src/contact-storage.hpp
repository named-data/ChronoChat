/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 *         Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_CONTACT_STORAGE_HPP
#define CHRONOCHAT_CONTACT_STORAGE_HPP

#include "contact.hpp"
#include "endorse-collection.hpp"
#include <sqlite3.h>

namespace chronochat {

class ContactStorage
{

public:
  class Error : public std::runtime_error
  {
  public:
    Error(const std::string &what)
      : std::runtime_error(what)
    {
    }
  };

  ContactStorage(const Name& identity);

  ~ContactStorage()
  {
    sqlite3_close(m_db);
  }

  shared_ptr<Profile>
  getSelfProfile();

  void
  addSelfEndorseCertificate(const EndorseCertificate& endorseCertificate);

  void
  addEndorseCertificate(const EndorseCertificate& endorseCertificate, const Name& identity);

  void
  updateCollectEndorse(const EndorseCertificate& endorseCertificate);

  void
  getCollectEndorse(EndorseCollection& endorseCollection);

  void
  getEndorseList(const Name& identity, std::vector<std::string>& endorseList);

  void
  removeContact(const Name& identity);

  void
  addContact(const Contact& contact);

  shared_ptr<Contact>
  getContact(const Name& identity) const;

  void
  updateIsIntroducer(const Name& identity, bool isIntroducer);

  void
  updateAlias(const Name& identity, const std::string& alias);

  void
  getAllContacts(std::vector<shared_ptr<Contact> >& contacts) const;

  void
  updateDnsSelfProfileData(const Data& data)
  {
    updateDnsData(data.wireEncode(), "N/A", "PROFILE", data.getName().toUri());
  }

  void
  updateDnsEndorseOthers(const Data& data, const std::string& endorsee)
  {
    updateDnsData(data.wireEncode(), endorsee, "ENDORSEE", data.getName().toUri());
  }

  void
  updateDnsOthersEndorse(const Data& data)
  {
    updateDnsData(data.wireEncode(), "N/A", "ENDORSED", data.getName().toUri());
  }

  shared_ptr<Data>
  getDnsData(const Name& name);

  shared_ptr<Data>
  getDnsData(const std::string& name, const std::string& type);

private:
  std::string
  getDBName();

  void
  initializeTable(const std::string& tableName, const std::string& sqlCreateStmt);

  bool
  doesContactExist(const Name& name);

  void
  updateDnsData(const Block& data,
                const std::string& name,
                const std::string& type,
                const std::string& dataName);

private:
  Name m_identity;

  sqlite3 *m_db;
};

} // namespace chronochat

#endif // CHRONOCHAT_CONTACT_STORAGE_HPP
