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
#include <ndn.cxx/regex/regex.h>

class ChatPolicyRule : public ndn::security::PolicyRule
{
  
public:
  ChatPolicyRule(ndn::Ptr<ndn::Regex> dataRegex,
                 ndn::Ptr<ndn::Regex> signerRegex);

  ChatPolicyRule(const ChatPolicyRule& rule);

  virtual
  ~ChatPolicyRule() {};

  bool 
  matchDataName(const ndn::Data & data);

  bool 
  matchSignerName(const ndn::Data & data);

  bool
  satisfy(const ndn::Data & data);

  bool
  satisfy(const ndn::Name & dataName, const ndn::Name & signerName);
  
private:
  ndn::Ptr<ndn::Regex> m_dataRegex;
  ndn::Ptr<ndn::Regex> m_signerRegex;
};

#endif //CHAT_POLICY_RULE_H
