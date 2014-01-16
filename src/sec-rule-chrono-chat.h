/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef SEC_RULE_CHRONO_CHAT_H
#define SEC_RULE_CHRONO_CHAT_H

#include <ndn-cpp-et/policy/sec-rule.hpp>
#include <ndn-cpp-et/regex/regex.hpp>

class SecRuleChronoChat : public ndn::SecRule
{
  
public:
  SecRuleChronoChat(ndn::ptr_lib::shared_ptr<ndn::Regex> dataRegex,
                    ndn::ptr_lib::shared_ptr<ndn::Regex> signerRegex);

  SecRuleChronoChat(const SecRuleChronoChat& rule);

  virtual
  ~SecRuleChronoChat() {};

  bool 
  matchDataName(const ndn::Data & data);

  bool 
  matchSignerName(const ndn::Data & data);

  bool
  satisfy(const ndn::Data & data);

  bool
  satisfy(const ndn::Name & dataName, const ndn::Name & signerName);
  
private:
  ndn::ptr_lib::shared_ptr<ndn::Regex> m_dataRegex;
  ndn::ptr_lib::shared_ptr<ndn::Regex> m_signerRegex;
};

#endif //CHAT_POLICY_RULE_H
