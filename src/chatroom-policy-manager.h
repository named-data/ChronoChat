/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHATROOM_POLICY_MANAGER_H
#define CHATROOM_POLICY_MANAGER_H

#include <ndn.cxx/security/policy/policy-manager.h>
#include <ndn.cxx/security/policy/identity-policy-rule.h>
#include <ndn.cxx/security/cache/certificate-cache.h>
#include <ndn.cxx/regex/regex.h>
#include <map>

#include "endorse-certificate.h"

class ChatroomPolicyManager : public ndn::security::PolicyManager
{
public:
  ChatroomPolicyManager(int stepLimit = 10,
                        ndn::Ptr<ndn::security::CertificateCache> certificateCache = NULL);
  
  virtual
  ~ChatroomPolicyManager();

  bool 
  skipVerifyAndTrust (const ndn::Data& data);

  bool
  requireVerify (const ndn::Data& data);

  ndn::Ptr<ndn::security::ValidationRequest>
  checkVerificationPolicy(ndn::Ptr<ndn::Data> data, 
                          const int& stepCount, 
                          const ndn::DataCallback& verifiedCallback,
                          const ndn::UnverifiedCallback& unverifiedCallback);

  bool 
  checkSigningPolicy(const ndn::Name& dataName, 
                     const ndn::Name& certificateName);
    
  ndn::Name 
  inferSigningIdentity(const ndn::Name& dataName);

  void
  addTrustAnchor(const EndorseCertificate& selfEndorseCertificate);

private:
  void 
  onCertificateVerified(ndn::Ptr<ndn::Data> certData, 
                        ndn::Ptr<ndn::Data> originalData,
                        const ndn::DataCallback& verifiedCallback, 
                        const ndn::UnverifiedCallback& unverifiedCallback);

  void
  onCertificateUnverified(ndn::Ptr<ndn::Data> certData, 
                          ndn::Ptr<ndn::Data> originalData,
                          const ndn::UnverifiedCallback& unverifiedCallback);

private:
  int m_stepLimit;
  ndn::Ptr<ndn::security::CertificateCache> m_certificateCache;

  ndn::Ptr<ndn::security::IdentityPolicyRule> m_invitationPolicyRule;
  ndn::Ptr<ndn::security::IdentityPolicyRule> m_dskRule;

  ndn::Ptr<ndn::Regex> m_keyNameRegex;

  std::map<ndn::Name, ndn::security::Publickey> m_trustAnchors;

};

#endif //CHATROOM_POLICY_MANAGER_H
