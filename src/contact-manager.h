/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef LINKNDN_CONTACT_MANAGER_H
#define LINKNDN_CONTACT_MANAGER_H

#include "contact-storage.h"
#include "ndn.cxx/wrapper/wrapper.h"

class ContactManager
{
public:
  ContactManager(ndn::Ptr<ContactStorage> contactStorage);

  ~ContactManager();

  inline ndn::Ptr<ContactStorage>
  getContactStorage()
  { return m_contactStorage; }

  inline ndn::Ptr<ndn::Wrapper>
  getWrapper()
  { return m_wrapper; }

private:
  ndn::Ptr<ndn::security::Keychain>
  setKeychain();
  
private:
  ndn::Ptr<ContactStorage> m_contactStorage;
  ndn::Ptr<ndn::Wrapper> m_wrapper;
};

#endif
