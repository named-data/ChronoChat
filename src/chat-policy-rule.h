/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHAT_POLICY_RULE_H
#define CHAT_POLICY_RULE_H

#include <ndn.cxx/security/policy/policy-rule.h>

class ChatPolicyRule : public ndn::security::PolicyRule
{
  
public:
  ChatPolicyRule();

  virtual
  ~ChatPolicyRyle() {};

  bool 
  matchDataName(const Data & data);

  bool 
  matchSignerName(const Data & data);

  bool
  satisfy(const Data & data);

  bool
  satisfy(const Name & dataName, const Name & signerName);
  
  TiXmlElement *
  toXmlElement();

private:
  ndn::Ptr<ndn::Regex> m_dataRegex;
  ndn::Ptr<ndn::Regex> m_signerRegex;
  ndn::Name m_dataRef;
  ndn::Name m_signerRef;
};

#endif //CHAT_POLICY_RULE_H
