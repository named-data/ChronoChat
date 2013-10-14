/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CONTACT_ITEM_H
#define CONTACT_ITEM_H

#include <ndn.cxx/data.h>
#include <vector>
#include "endorse-certificate.h"

class ContactItem
{
  typedef std::vector<Ptr<EndorseCertificate> > EndorseCertificateList;

public:
  ContactItem(const EndorseCertificate& selfEndorseCertificate,
              const std::string& alias = std::string());
  
  ~ContactItem() {}

  const ndn::Name&
  getNameSpace() const
  { return m_namespace; }

  const std::string&
  getAlias() const
  { return m_alias; }

  const std::string&
  getName() const
  { return m_name; }

  const std::string&
  getInstitution() const
  { return m_institution; }

private:
  EndorseCertificate m_selfEndorseCertificate;

  ndn::Name m_namespace;
  std::string m_alias;

  std::string m_name;
  std::string m_institution;
};

#endif
