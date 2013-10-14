/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef LINKNDN_CONTACT_ITEM_H
#define LINKNDN_CONTACT_ITEM_H

#include <ndn.cxx/data.h>
#include <vector>
#include "endorse-certificate.h"

class ContactItem
{
  typedef std::vector<ndn::Ptr<EndorseCertificate> > EndorseCertificateList;

public:
  ContactItem(const EndorseCertificate& selfEndorseCertificate,
              const std::string& alias = std::string());
  
  ~ContactItem() {}

  inline const EndorseCertificate&
  getSelfEndorseCertificate() const
  { return m_selfEndorseCertificate; }

  inline const ndn::Name&
  getNameSpace() const
  { return m_namespace; }

  inline const std::string&
  getAlias() const
  { return m_alias; }

  inline const std::string&
  getName() const
  { return m_name; }

  inline const std::string&
  getInstitution() const
  { return m_institution; }

  inline const ndn::Name
  getPublicKeyName() const
  { return m_selfEndorseCertificate.getPublicKeyName(); }

protected:
  EndorseCertificate m_selfEndorseCertificate;

  ndn::Name m_namespace;
  std::string m_alias;

  std::string m_name;
  std::string m_institution;
};

#endif
