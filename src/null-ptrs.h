/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef NULL_PTRS_H
#define NULL_PTRS_H

#include <string>
#include <ndn-cpp/util/blob.hpp>
#include <ndn-cpp/data.hpp>
#include "contact-item.h"
#include "endorse-certificate.h"
#include "profile.h"
#include <ndn-cpp/security/certificate/identity-certificate.hpp>
#include <ndn-cpp/security/policy/validation-request.hpp>
#include <ndn-cpp/security/certificate/certificate.hpp>


static std::string CHRONOCHAT_NULL_STR;
static ndn::Blob CHRONOCHAT_NULL_BLOB;

static ndn::ptr_lib::shared_ptr<ContactItem> CHRONOCHAT_NULL_CONTACTITEM_PTR;
static ndn::ptr_lib::shared_ptr<ndn::Data> CHRONOCHAT_NULL_DATA_PTR;
static ndn::ptr_lib::shared_ptr<EndorseCertificate> CHRONOCHAT_NULL_ENDORSECERTIFICATE_PTR;
static ndn::ptr_lib::shared_ptr<ndn::IdentityCertificate> CHRONOCHAT_NULL_IDENTITYCERTIFICATE_PTR;
static ndn::ptr_lib::shared_ptr<Profile> CHRONOCHAT_NULL_PROFILE_PTR;
static ndn::ptr_lib::shared_ptr<ndn::PublicKey> CHRONOCHAT_NULL_PUBLICKEY_PTR;
static ndn::ptr_lib::shared_ptr<ndn::ValidationRequest> CHRONOCHAT_NULL_VALIDATIONREQUEST_PTR;
static ndn::ptr_lib::shared_ptr<const ndn::Certificate> CHRONOCHAT_NULL_CONST_CERTIFICATE_PTR;

#endif
