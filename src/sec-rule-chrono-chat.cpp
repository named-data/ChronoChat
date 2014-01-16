/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "sec-rule-chrono-chat.h"
#include <ndn-cpp/security/signature-sha256-with-rsa.hpp>

using namespace ndn;
using namespace std;
using namespace ndn::ptr_lib;


SecRuleChronoChat::SecRuleChronoChat(shared_ptr<Regex> dataRegex,
                                     shared_ptr<Regex> signerRegex)
  : SecRule(SecRule::IDENTITY_RULE, true)
  , m_dataRegex(dataRegex)
  , m_signerRegex(signerRegex)
{}

SecRuleChronoChat::SecRuleChronoChat(const SecRuleChronoChat& rule)
  : SecRule(SecRule::IDENTITY_RULE, true)
  , m_dataRegex(rule.m_dataRegex)
  , m_signerRegex(rule.m_signerRegex)
{}

bool 
SecRuleChronoChat::matchDataName(const Data & data)
{ return m_dataRegex->match(data.getName()); }

bool 
SecRuleChronoChat::matchSignerName(const Data & data)
{ 
  try{
    SignatureSha256WithRsa sig(data.getSignature());
    Name signerName = sig.getKeyLocator().getName ();
    return m_signerRegex->match(signerName); 
  }catch(SignatureSha256WithRsa::Error &e){
    return false;
  }catch(KeyLocator::Error &e){
    return false;
  }
}

bool
SecRuleChronoChat::satisfy(const Data & data)
{ return (matchDataName(data) && matchSignerName(data)) ? true : false ; }

bool
SecRuleChronoChat::satisfy(const Name & dataName, const Name & signerName)
{ return (m_dataRegex->match(dataName) && m_signerRegex->match(signerName)); }
