/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "chat-policy-rule.cpp"

using namespace ndn;
using namespace std;
using namespace ndn::security;


ChatPolicyRule::ChatPolicyRule(Ptr<Regex> dataRegex, const Name& dataRef,
			       Ptr<Regex> signerRegex, const Name& signerRef,
			       bool isPositive)
  : PolicyRule(PolicyRule::IDENTITY_POLICY, isPositive)
  , m_dataRegex(dataRegex)
  , m_signerRegex(signerRegex)
  , m_dataRef(dataRef)
  , m_signerRef(signerRef)
{}

bool 
ChatPolicyRule::matchDataName(const Data & data)
{ return (m_dataRegex.match(data.getName()) && m_dataRegex.expand() == m_dataRef) ? true : false; }

bool 
ChatPolicyRule::matchSignerName(const Data & data)
{ 
  Ptr<const signature::Sha256WithRsa> sigPtr = DynamicCast<const signature::Sha256WithRsa> (data.getSignature());
  Name signerName = sigPtr->getKeyLocator ().getKeyName ();
  return (m_signerRegex.match(signerName) && m_signerRegex.expand() == m_signerRef) ? true : false; 
}

bool
ChatPolicyRule::satisfy(const Data & data)
{ return (matchDataName(data) && matchSignerName(data)) ? true : false ; }

bool
ChatPolicyRule::satisfy(const Name & dataName, const Name & signerName)
{
  if (m_dataRegex.match(data.getName()) 
      && m_dataRegex.expand() == m_dataRef
      && m_signerRegex.match(signerName) 
      && m_signerRegex.expand() == m_signerRef)
    return true;
  else
    return false;
}
  
TiXmlElement *
ChatPolicyRule::toXmlElement()
{
  //TODO:
  return NULL;
}
};
