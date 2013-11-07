/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "chat-policy-rule.h"
#include <ndn.cxx/fields/signature-sha256-with-rsa.h>

using namespace ndn;
using namespace std;
using namespace ndn::security;


ChatPolicyRule::ChatPolicyRule(Ptr<Regex> dataRegex,
			       Ptr<Regex> signerRegex)
  : PolicyRule(PolicyRule::IDENTITY_POLICY, true)
  , m_dataRegex(dataRegex)
  , m_signerRegex(signerRegex)
{}

ChatPolicyRule::ChatPolicyRule(const ChatPolicyRule& rule)
  : PolicyRule(PolicyRule::IDENTITY_POLICY, true)
  , m_dataRegex(rule.m_dataRegex)
  , m_signerRegex(rule.m_signerRegex)
{}

bool 
ChatPolicyRule::matchDataName(const Data & data)
{ return m_dataRegex->match(data.getName()); }

bool 
ChatPolicyRule::matchSignerName(const Data & data)
{ 
  Ptr<const Signature> sig = data.getSignature();

  if(NULL == sig)
    return false;

  Ptr<const signature::Sha256WithRsa> sigPtr = DynamicCast<const signature::Sha256WithRsa> (sig);
  if(KeyLocator::KEYNAME != sigPtr->getKeyLocator().getType())
    return false;

  Name signerName = sigPtr->getKeyLocator ().getKeyName ();
  return m_signerRegex->match(signerName); 
}

bool
ChatPolicyRule::satisfy(const Data & data)
{ return (matchDataName(data) && matchSignerName(data)) ? true : false ; }

bool
ChatPolicyRule::satisfy(const Name & dataName, const Name & signerName)
{ return (m_dataRegex->match(dataName) && m_signerRegex->match(signerName)); }
