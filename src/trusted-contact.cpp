/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "trusted-contact.h"
#include <tinyxml.h>

using namespace std;
using namespace ndn;

TrustedContact::TrustedContact(const EndorseCertificate& selfEndorseCertificate,
			       const string& trustScope,
			       const string& alias)
  : ContactItem(selfEndorseCertificate, alias)
{
  TiXmlDocument xmlDoc;
  xmlDoc.Parse(trustScope.c_str());
  
  TiXmlNode * it = xmlDoc.FirstChild();    
  while(it != NULL)
    {
      m_trustScope.push_back(Regex::fromXmlElement(dynamic_cast<TiXmlElement *>(it)));
      it = it->NextSibling();
    }
}

bool 
TrustedContact::canBeTrustedFor(const Name& name)
{
  vector<Ptr<Regex> >::iterator it = m_trustScope.begin();

  for(; it != m_trustScope.end(); it++)
    if((*it)->match(name))
      return true;
  return false;
}

Ptr<Blob> 
TrustedContact::getTrustScopeBlob() const
{
  ostringstream oss;
  TiXmlDocument * xmlDoc = new TiXmlDocument();

  vector<Ptr<Regex> >::const_iterator it = m_trustScope.begin();
  for(; it != m_trustScope.end(); it++)
      xmlDoc->LinkEndChild((*it)->toXmlElement());

  oss << *xmlDoc;
  return Ptr<Blob>(new Blob(oss.str().c_str(), oss.str().size()));
}
