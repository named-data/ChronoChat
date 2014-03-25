/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#if __clang__
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif

#include "contact-manager.h"
#include <QStringList>
#include <QFile>

#ifndef Q_MOC_RUN
#include <ndn-cpp-dev/util/crypto.hpp>
#include <ndn-cpp-dev/util/io.hpp>
#include <ndn-cpp-dev/security/sec-rule-relative.hpp>
#include <ndn-cpp-dev/security/validator-regex.hpp>
#include <cryptopp/base64.h>
#include <cryptopp/files.h>
#include <cryptopp/sha.h>
#include <cryptopp/filters.h>
#include <boost/asio.hpp>
#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>
#include "logging.h"
#endif

using namespace ndn;
using namespace std;
namespace fs = boost::filesystem;

INIT_LOGGER("chronos.ContactManager");

namespace chronos{

static const uint8_t DNS_RP_SEPARATOR[2] = {0xF0, 0x2E}; // %F0.

ContactManager::ContactManager(shared_ptr<Face> face,
                               QObject* parent)
  : QObject(parent)
  , m_face(face)
  , m_dnsListenerId(0)
{
  initializeSecurity();
}

ContactManager::~ContactManager()
{}

// private methods
shared_ptr<IdentityCertificate>
ContactManager::loadTrustAnchor()
{
  shared_ptr<IdentityCertificate> anchor;

  QFile anchorFile(":/security/anchor.cert");

  if (!anchorFile.open(QIODevice::ReadOnly))
    {
      emit warning(QString("Cannot load trust anchor!"));

      return anchor;
    }

  qint64 fileSize = anchorFile.size();
  char* buf = new char[fileSize];
  anchorFile.read(buf, fileSize);

  try
    {
      using namespace CryptoPP;

      OBufferStream os;
      StringSource(reinterpret_cast<const uint8_t*>(buf), fileSize, true, new Base64Decoder(new FileSink(os)));
      anchor = make_shared<IdentityCertificate>();
      anchor->wireDecode(Block(os.buf()));
    }
  catch(CryptoPP::Exception& e)
    {
      emit warning(QString("Cannot load trust anchor!"));
    }
  catch(IdentityCertificate::Error& e)
    {
      emit warning(QString("Cannot load trust anchor!"));
    }
  catch(Block::Error& e)
    {
      emit warning(QString("Cannot load trust anchor!"));
    }

  delete [] buf;

  return anchor;
}

void
ContactManager::initializeSecurity()
{
  shared_ptr<IdentityCertificate> anchor = loadTrustAnchor();

  shared_ptr<ValidatorRegex> validator = make_shared<ValidatorRegex>(m_face);
  validator->addDataVerificationRule(make_shared<SecRuleRelative>("^([^<DNS>]*)<DNS><ENDORSED>",
                                                                  "^([^<KEY>]*)<KEY>(<>*)<><ID-CERT>$",
                                                                  "==", "\\1", "\\1\\2", true));
  validator->addDataVerificationRule(make_shared<SecRuleRelative>("^([^<DNS>]*)<DNS><><ENDORSEE>",
                                                                  "^([^<KEY>]*)<KEY>(<>*)<><ID-CERT>$",
                                                                  "==", "\\1", "\\1\\2", true));
  validator->addDataVerificationRule(make_shared<SecRuleRelative>("^([^<DNS>]*)<DNS><PROFILE>",
                                                                  "^([^<KEY>]*)<KEY>(<>*)<><ID-CERT>$",
                                                                  "==", "\\1", "\\1\\2", true));
  validator->addDataVerificationRule(make_shared<SecRuleRelative>("^([^<PROFILE-CERT>]*)<PROFILE-CERT>",
                                                                  "^([^<KEY>]*)<KEY>(<>*<ksk-.*>)<ID-CERT>$", 
                                                                  "==", "\\1", "\\1\\2", true));
  validator->addDataVerificationRule(make_shared<SecRuleRelative>("^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>",
                                                                  "^([^<KEY>]*)<KEY><dsk-.*><ID-CERT>$",
                                                                  ">", "\\1\\2", "\\1", true));
  validator->addDataVerificationRule(make_shared<SecRuleRelative>("^([^<KEY>]*)<KEY><dsk-.*><ID-CERT>",
                                                                  "^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>$",
                                                                  "==", "\\1", "\\1\\2", true));
  validator->addDataVerificationRule(make_shared<SecRuleRelative>("^(<>*)$", 
                                                                  "^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>$", 
                                                                  ">", "\\1", "\\1\\2", true));
  validator->addTrustAnchor(anchor);
  m_validator = validator;
}

void
ContactManager::fetchCollectEndorse(const Name& identity)
{
  Name interestName = identity;
  interestName.append("DNS").append("ENDORSED");

  Interest interest(interestName);
  interest.setInterestLifetime(time::milliseconds(1000));
  interest.setMustBeFresh(true);

  OnDataValidated onValidated = bind(&ContactManager::onDnsCollectEndorseValidated, this, _1, identity);
  OnDataValidationFailed onValidationFailed = bind(&ContactManager::onDnsCollectEndorseValidationFailed, this, _1, _2, identity);
  TimeoutNotify timeoutNotify = bind(&ContactManager::onDnsCollectEndorseTimeoutNotify, this, _1, identity);

  sendInterest(interest, onValidated, onValidationFailed, timeoutNotify, 0);
}

void
ContactManager::fetchEndorseCertificateInternal(const Name& identity, int certIndex)
{
  shared_ptr<EndorseCollection> endorseCollection = m_bufferedContacts[identity].m_endorseCollection;

  if(certIndex >= endorseCollection->endorsement_size())
    prepareEndorseInfo(identity);

  Name interestName(endorseCollection->endorsement(certIndex).certname());

  Interest interest(interestName);
  interest.setInterestLifetime(time::milliseconds(1000));
  interest.setMustBeFresh(true);

  m_face->expressInterest(interest,
                          bind(&ContactManager::onEndorseCertificateInternal, 
                               this, _1, _2, identity, certIndex,
                               endorseCollection->endorsement(certIndex).hash()),
                          bind(&ContactManager::onEndorseCertificateInternalTimeout,
                               this, _1, identity, certIndex));
}

void
ContactManager::prepareEndorseInfo(const Name& identity)
{
  // _LOG_DEBUG("prepareEndorseInfo");
  const Profile& profile = m_bufferedContacts[identity].m_selfEndorseCert->getProfile();

  shared_ptr<EndorseInfo> endorseInfo = make_shared<EndorseInfo>();
  m_bufferedContacts[identity].m_endorseInfo = endorseInfo;

  Profile::const_iterator pIt  = profile.begin();
  Profile::const_iterator pEnd = profile.end();

  map<string, int> endorseCount;
  for(; pIt != pEnd; pIt++)
    {
      // _LOG_DEBUG("prepareEndorseInfo: profile[" << pIt->first << "]: " << pIt->second);
      endorseCount[pIt->first] = 0;
    }

  int endorseCertCount = 0;

  vector<shared_ptr<EndorseCertificate> >::const_iterator cIt  = m_bufferedContacts[identity].m_endorseCertList.begin();
  vector<shared_ptr<EndorseCertificate> >::const_iterator cEnd = m_bufferedContacts[identity].m_endorseCertList.end();

  for(; cIt != cEnd; cIt++, endorseCertCount++)
    {
      shared_ptr<Contact> contact = getContact((*cIt)->getSigner().getPrefix(-1));
      if(!static_cast<bool>(contact))
        continue;
      
      if(!contact->isIntroducer() 
         || !contact->canBeTrustedFor(profile.getIdentityName()))
        continue;

      if(!Validator::verifySignature(**cIt, contact->getPublicKey()))
        continue;

      const Profile& tmpProfile = (*cIt)->getProfile();
      if(tmpProfile != profile)
        continue;

      const vector<string>& endorseList = (*cIt)->getEndorseList();
      vector<string>::const_iterator eIt = endorseList.begin();
      for(; eIt != endorseList.end(); eIt++)
        endorseCount[*eIt] += 1;
    }

  pIt  = profile.begin();
  pEnd = profile.end();
  for(; pIt != pEnd; pIt++)
    {
      EndorseInfo::Endorsement* endorsement = endorseInfo->add_endorsement();
      endorsement->set_type(pIt->first);
      endorsement->set_value(pIt->second);
      stringstream ss;
      ss << endorseCount[pIt->first] << "/" << endorseCertCount;
      endorsement->set_endorse(ss.str());
    }

  emit contactEndorseInfoReady (*endorseInfo);
}

void
ContactManager::onDnsSelfEndorseCertValidated(const shared_ptr<const Data>& data, 
                                              const Name& identity)
{
  try
    {
      Data plainData;
      plainData.wireDecode(data->getContent().blockFromValue());
      shared_ptr<EndorseCertificate> selfEndorseCertificate = make_shared<EndorseCertificate>(boost::cref(plainData));
      if(Validator::verifySignature(plainData, selfEndorseCertificate->getPublicKeyInfo()))
        {
          m_bufferedContacts[identity].m_selfEndorseCert = selfEndorseCertificate;
          fetchCollectEndorse(identity);
        }
      else
        emit contactInfoFetchFailed(QString::fromStdString(identity.toUri()));
    }
  catch(Block::Error& e)
    {
      emit contactInfoFetchFailed(QString::fromStdString(identity.toUri()));
    }
  catch(Data::Error& e)
    {
      emit contactInfoFetchFailed(QString::fromStdString(identity.toUri()));
    }
  catch(EndorseCertificate::Error& e)
    {
      emit contactInfoFetchFailed(QString::fromStdString(identity.toUri()));
    }
}

void
ContactManager::onDnsSelfEndorseCertValidationFailed(const shared_ptr<const Data>& data, 
                                                     const string& failInfo,
                                                     const Name& identity)
{
  // If we cannot validate the Self-Endorse-Certificate, we may retry or fetch id-cert,
  // but let's stay with failure for now.
  emit contactInfoFetchFailed(QString::fromStdString(identity.toUri()));
}

void
ContactManager::onDnsSelfEndorseCertTimeoutNotify(const Interest& interest,
                                                  const Name& identity)
{
  // If we cannot validate the Self-Endorse-Certificate, we may retry or fetch id-cert,
  // but let's stay with failure for now.
  emit contactInfoFetchFailed(QString::fromStdString(identity.toUri()));
}

void
ContactManager::onDnsCollectEndorseValidated(const shared_ptr<const Data>& data, 
                                             const Name& identity)
{
  shared_ptr<EndorseCollection> endorseCollection = make_shared<EndorseCollection>();
  if(!endorseCollection->ParseFromArray(data->getContent().value(), data->getContent().value_size()))
    {
      m_bufferedContacts[identity].m_endorseCollection = endorseCollection;
      fetchEndorseCertificateInternal(identity, 0);
    }
  else
    prepareEndorseInfo(identity);
}

void
ContactManager::onDnsCollectEndorseValidationFailed(const shared_ptr<const Data>& data, 
                                                    const string& failInfo,
                                                    const Name& identity)
{
  prepareEndorseInfo(identity);
}

void
ContactManager::onDnsCollectEndorseTimeoutNotify(const Interest& interest,
                                                 const Name& identity)
{
  // _LOG_DEBUG("onDnsCollectEndorseTimeoutNotify: " << interest.getName());
  prepareEndorseInfo(identity);
}

void
ContactManager::onEndorseCertificateInternal(const Interest& interest,
                                             Data& data, 
                                             const Name& identity, 
                                             int certIndex,
                                             string hash)
{
  stringstream ss;
  {  
    using namespace CryptoPP;

    SHA256 hash;
    StringSource(data.wireEncode().wire(), data.wireEncode().size(), true,
                 new HashFilter(hash, new FileSink(ss)));
  }
  
  if(ss.str() == hash)
    {
      shared_ptr<EndorseCertificate> endorseCertificate = make_shared<EndorseCertificate>(boost::cref(data));
      m_bufferedContacts[identity].m_endorseCertList.push_back(endorseCertificate);
    }

  fetchEndorseCertificateInternal(identity, certIndex+1);
}

void
ContactManager::onEndorseCertificateInternalTimeout(const Interest& interest,
                                                    const Name& identity, 
                                                    int certIndex)
{
  fetchEndorseCertificateInternal(identity, certIndex+1);
}

void
ContactManager::collectEndorsement()
{
  {
    boost::recursive_mutex::scoped_lock lock(m_collectCountMutex);
    m_collectCount = m_contactList.size();
    
    ContactList::iterator it  = m_contactList.begin();
    ContactList::iterator end = m_contactList.end();

    for(; it != end ; it++)
      {
        Name interestName = (*it)->getNameSpace();
        interestName.append("DNS").append(m_identity.wireEncode()).append("ENDORSEE");
        
        Interest interest(interestName);
        interest.setInterestLifetime(time::milliseconds(1000));
        
        OnDataValidated onValidated = bind(&ContactManager::onDnsEndorseeValidated, this, _1);
        OnDataValidationFailed onValidationFailed = bind(&ContactManager::onDnsEndorseeValidationFailed, this, _1, _2);
        TimeoutNotify timeoutNotify = bind(&ContactManager::onDnsEndorseeTimeoutNotify, this, _1);
        
        sendInterest(interest, onValidated, onValidationFailed, timeoutNotify, 0);
      }
  }
}

void
ContactManager::onDnsEndorseeValidated(const shared_ptr<const Data>& data)
{
  Data endorseData;
  endorseData.wireDecode(data->getContent().blockFromValue());

  EndorseCertificate endorseCertificate(endorseData);
  m_contactStorage->updateCollectEndorse(endorseCertificate);

  decreaseCollectStatus();
}

void
ContactManager::onDnsEndorseeValidationFailed(const shared_ptr<const Data>& data, const string& failInfo)
{
  decreaseCollectStatus();
}

void
ContactManager::onDnsEndorseeTimeoutNotify(const Interest& interest)
{
  decreaseCollectStatus();
}

void 
ContactManager::decreaseCollectStatus()
{
  int count; 
  {
    boost::recursive_mutex::scoped_lock lock(m_collectCountMutex);
    m_collectCount--;
    count = m_collectCount;
  }

  if(count == 0)
    publishCollectEndorsedDataInDNS();
}

void
ContactManager::publishCollectEndorsedDataInDNS()
{
  Name dnsName = m_identity;
  dnsName.append("DNS").append("ENDORSED").appendVersion();

  Data data;
  data.setName(dnsName);

  EndorseCollection endorseCollection;
  m_contactStorage->getCollectEndorse(endorseCollection);

  OBufferStream os;
  endorseCollection.SerializeToOstream(&os);

  data.setContent(os.buf());  
  m_keyChain.signByIdentity(data, m_identity);

  m_contactStorage->updateDnsOthersEndorse(data);
  m_face->put(data);
}

void
ContactManager::onIdentityCertValidated(const shared_ptr<const Data>& data)
{
  shared_ptr<IdentityCertificate> cert = make_shared<IdentityCertificate>(boost::cref(*data));
  m_bufferedIdCerts[cert->getName()] = cert;
  decreaseIdCertCount();
}

void
ContactManager::onIdentityCertValidationFailed(const shared_ptr<const Data>& data, const string& failInfo)
{
  _LOG_DEBUG("ContactManager::onIdentityCertValidationFailed " << data->getName());
  decreaseIdCertCount();
}

void
ContactManager::onIdentityCertTimeoutNotify(const Interest& interest)
{
  _LOG_DEBUG("ContactManager::onIdentityCertTimeoutNotify: " << interest.getName());
  decreaseIdCertCount();
}

void
ContactManager::decreaseIdCertCount()
{
  int count;
  {
    boost::recursive_mutex::scoped_lock lock(m_idCertCountMutex);
    m_idCertCount--;
    count = m_idCertCount;
  }

  if(count == 0)
    {
      QStringList certNameList;
      QStringList nameList;

      BufferedIdCerts::const_iterator it  = m_bufferedIdCerts.begin();
      BufferedIdCerts::const_iterator end = m_bufferedIdCerts.end();
      for(; it != end; it++)
        {
          certNameList << QString::fromStdString(it->second->getName().toUri());
          Profile profile(*(it->second));
          nameList << QString::fromStdString(profile.get("name"));
        }

      emit idCertNameListReady(certNameList);
      emit nameListReady(nameList);
    }
}

shared_ptr<EndorseCertificate>
ContactManager::getSignedSelfEndorseCertificate(const Profile& profile)
{
  Name certificateName = m_keyChain.getDefaultCertificateNameForIdentity(m_identity);

  shared_ptr<IdentityCertificate> signingCert = m_keyChain.getCertificate(certificateName);

  vector<string> endorseList;
  Profile::const_iterator it = profile.begin();
  for(; it != profile.end(); it++)
    endorseList.push_back(it->first);
  
  shared_ptr<EndorseCertificate> selfEndorseCertificate = 
    shared_ptr<EndorseCertificate>(new EndorseCertificate(*signingCert, profile, endorseList));
  
  m_keyChain.sign(*selfEndorseCertificate, certificateName);
  
  return selfEndorseCertificate;
}

void
ContactManager::publishSelfEndorseCertificateInDNS(const EndorseCertificate& selfEndorseCertificate)
{
  Name dnsName = m_identity;
  dnsName.append("DNS").append("PROFILE").appendVersion();

  Data data;
  data.setName(dnsName);
  data.setContent(selfEndorseCertificate.wireEncode());
  data.setFreshnessPeriod(time::milliseconds(1000));

  m_keyChain.signByIdentity(data, m_identity);

  m_contactStorage->updateDnsSelfProfileData(data);
  m_face->put(data);
}

shared_ptr<EndorseCertificate> 
ContactManager::generateEndorseCertificate(const Name& identity)
{
  shared_ptr<Contact> contact = getContact(identity);
  if(!static_cast<bool>(contact))
    return shared_ptr<EndorseCertificate>();

  Name signerKeyName = m_keyChain.getDefaultKeyNameForIdentity(m_identity);

  vector<string> endorseList;
  m_contactStorage->getEndorseList(identity, endorseList);

  shared_ptr<EndorseCertificate> cert = 
    shared_ptr<EndorseCertificate>(new EndorseCertificate(contact->getPublicKeyName(), 
                                                          contact->getPublicKey(),
                                                          contact->getNotBefore(),
                                                          contact->getNotAfter(),
                                                          signerKeyName, 
                                                          contact->getProfile(), 
                                                          endorseList)); 
  m_keyChain.signByIdentity(*cert, m_identity);
  return cert;

}

void
ContactManager::publishEndorseCertificateInDNS(const EndorseCertificate& endorseCertificate)
{
  Name endorsee = endorseCertificate.getPublicKeyName().getPrefix(-1);
  Name dnsName = m_identity;
  dnsName.append("DNS")
    .append(endorsee.wireEncode())
    .append("ENDORSEE")
    .appendVersion();

  Data data;
  data.setName(dnsName);
  data.setContent(endorseCertificate.wireEncode());

  m_keyChain.signByIdentity(data, m_identity);

  m_contactStorage->updateDnsEndorseOthers(data, dnsName.get(-3).toEscapedString());
  m_face->put(data);
}

void
ContactManager::sendInterest(const Interest& interest,
                             const OnDataValidated& onValidated,
                             const OnDataValidationFailed& onValidationFailed,
                             const TimeoutNotify& timeoutNotify,
                             int retry /* = 1 */)
{
  m_face->expressInterest(interest, 
                          bind(&ContactManager::onTargetData, 
                               this, _1, _2, onValidated, onValidationFailed),
                          bind(&ContactManager::onTargetTimeout,
                               this, _1, retry, onValidated, onValidationFailed, timeoutNotify));
}

void
ContactManager::onTargetData(const Interest& interest, 
                             const Data& data,
                             const OnDataValidated& onValidated,
                             const OnDataValidationFailed& onValidationFailed)
{
  // _LOG_DEBUG("On receiving data: " << data.getName());
  m_validator->validate(data, onValidated, onValidationFailed); 
}

void
ContactManager::onTargetTimeout(const Interest& interest, 
                                int retry,
                                const OnDataValidated& onValidated,
                                const OnDataValidationFailed& onValidationFailed,
                                const TimeoutNotify& timeoutNotify)
{
  // _LOG_DEBUG("On interest timeout: " << interest.getName());
  if(retry > 0)
    sendInterest(interest, onValidated, onValidationFailed, timeoutNotify, retry-1);
  else
    timeoutNotify(interest);
}

void
ContactManager::onDnsInterest(const Name& prefix, const Interest& interest)
{
  const Name& interestName = interest.getName();
  shared_ptr<Data> data;
  
  if(interestName.size() <= prefix.size())
    return;

  if(interestName.size() == (prefix.size()+1))
    {
      data = m_contactStorage->getDnsData("N/A", interestName.get(prefix.size()).toEscapedString());
      if(static_cast<bool>(data))
        m_face->put(*data);
      return;
    }

  if(interestName.size() == (prefix.size()+2))
    {
      data = m_contactStorage->getDnsData(interestName.get(prefix.size()).toEscapedString(),
                                          interestName.get(prefix.size()+1).toEscapedString());
      if(static_cast<bool>(data))
        m_face->put(*data);
      return;
    }
}
  
void
ContactManager::onDnsRegisterFailed(const Name& prefix, const string& failInfo)
{
  emit warning(QString(failInfo.c_str()));
}


// public slots
void
ContactManager::onIdentityUpdated(const QString& identity)
{
  m_identity = Name(identity.toStdString());

  m_contactStorage = make_shared<ContactStorage>(m_identity);

  if(m_dnsListenerId)
    m_face->unsetInterestFilter(m_dnsListenerId);

  Name dnsPrefix;
  dnsPrefix.append(m_identity).append("DNS");
  m_dnsListenerId = m_face->setInterestFilter(dnsPrefix, 
                                              bind(&ContactManager::onDnsInterest, this, _1, _2),
                                              bind(&ContactManager::onDnsRegisterFailed, this, _1, _2));

  m_contactList.clear();
  m_contactStorage->getAllContacts(m_contactList);

  m_bufferedContacts.clear();

  collectEndorsement();
}

void
ContactManager::onFetchContactInfo(const QString& identity)
{
  // try to fetch self-endorse-certificate via DNS PROFILE first.
  Name identityName(identity.toStdString());
  Name interestName;
  interestName.append(identityName).append("DNS").append("PROFILE");

  // _LOG_DEBUG("onFetchContactInfo " << identity.toStdString() << " profile: " << interestName);
  
  Interest interest(interestName);
  interest.setInterestLifetime(time::milliseconds(1000));
  interest.setMustBeFresh(true);

  OnDataValidated onValidated = bind(&ContactManager::onDnsSelfEndorseCertValidated, this, _1, identityName);
  OnDataValidationFailed onValidationFailed = bind(&ContactManager::onDnsSelfEndorseCertValidationFailed, this, _1, _2, identityName);
  TimeoutNotify timeoutNotify = bind(&ContactManager::onDnsSelfEndorseCertTimeoutNotify, this, _1, identityName);

  sendInterest(interest, onValidated, onValidationFailed, timeoutNotify, 0);
}

void
ContactManager::onAddFetchedContact(const QString& identity)
{
  // _LOG_DEBUG("onAddFetchedContact");

  Name identityName(identity.toStdString());

  BufferedContacts::const_iterator it = m_bufferedContacts.find(identityName);
  if(it != m_bufferedContacts.end())
    {
      Contact contact(*(it->second.m_selfEndorseCert));
      // _LOG_DEBUG("onAddFetchedContact: contact ready");
      try
        {
          m_contactStorage->addContact(contact);
          m_bufferedContacts.erase(identityName);

          m_contactList.clear();
          m_contactStorage->getAllContacts(m_contactList);

          onWaitForContactList();
        }
      catch(ContactStorage::Error& e)
        {
          emit warning(QString::fromStdString(e.what()));
        }
    }
  else
    {
      emit warning(QString("Failure: no information of %1").arg(identity));
    }
}

void
ContactManager::onUpdateProfile()
{
  // Get current profile;
  shared_ptr<Profile> newProfile = m_contactStorage->getSelfProfile();
  if(!static_cast<bool>(newProfile))
    return;

  _LOG_DEBUG("ContactManager::onUpdateProfile: getProfile");

  shared_ptr<EndorseCertificate> newEndorseCertificate = getSignedSelfEndorseCertificate(*newProfile);

  m_contactStorage->addSelfEndorseCertificate(*newEndorseCertificate);

  publishSelfEndorseCertificateInDNS(*newEndorseCertificate);
}

void
ContactManager::onRefreshBrowseContact()
{
  std::vector<std::string> bufferedIdCertNames;
  try
    {
      using namespace boost::asio::ip;
      tcp::iostream request_stream;
      request_stream.expires_from_now(boost::posix_time::milliseconds(5000));
      request_stream.connect("ndncert.named-data.net","80");
      if(!request_stream)
        {
          emit warning(QString::fromStdString("Fail to fetch certificate directory! #1"));
          return;
        }

      request_stream << "GET /cert/list/ HTTP/1.0\r\n";
      request_stream << "Host: ndncert.named-data.net\r\n\r\n";
      request_stream.flush();

      string line1;
      std::getline(request_stream,line1);
      if (!request_stream)
        {
          emit warning(QString::fromStdString("Fail to fetch certificate directory! #2"));
          return;
        }

      std::stringstream response_stream(line1);
      std::string http_version;
      response_stream >> http_version;
      unsigned int status_code;
      response_stream >> status_code;
      std::string status_message;
      std::getline(response_stream,status_message);

      if (!response_stream||http_version.substr(0,5)!="HTTP/")
        {
          emit warning(QString::fromStdString("Fail to fetch certificate directory! #3"));
          return;
        }
      if (status_code!=200)
        {
          emit warning(QString::fromStdString("Fail to fetch certificate directory! #4"));
          return;
        }
      vector<string> headers;
      std::string header;
      while (std::getline(request_stream, header) && header != "\r")
        headers.push_back(header);

      std::istreambuf_iterator<char> stream_iter (request_stream);
      std::istreambuf_iterator<char> end_of_stream;
      
      typedef boost::tokenizer< boost::escaped_list_separator<char>, std::istreambuf_iterator<char> > tokenizer_t;
      tokenizer_t certItems (stream_iter, end_of_stream,boost::escaped_list_separator<char>('\\', '\n', '"'));

      for (tokenizer_t::iterator it = certItems.begin(); it != certItems.end (); it++)
        if (!it->empty())
          bufferedIdCertNames.push_back(*it);
    }
  catch(std::exception &e)
    {
      emit warning(QString::fromStdString("Fail to fetch certificate directory! #N"));
    }

  {
    boost::recursive_mutex::scoped_lock lock(m_idCertCountMutex);
    m_idCertCount = bufferedIdCertNames.size();
  }
  m_bufferedIdCerts.clear();

  std::vector<std::string>::const_iterator it  = bufferedIdCertNames.begin();
  std::vector<std::string>::const_iterator end = bufferedIdCertNames.end();
  for(; it != end; it++)
    {
      Name certName(*it);

      Interest interest(certName);
      interest.setInterestLifetime(time::milliseconds(1000));
      interest.setMustBeFresh(true);
      
      OnDataValidated onValidated = bind(&ContactManager::onIdentityCertValidated, this, _1);
      OnDataValidationFailed onValidationFailed = bind(&ContactManager::onIdentityCertValidationFailed, this, _1, _2);
      TimeoutNotify timeoutNotify = bind(&ContactManager::onIdentityCertTimeoutNotify, this, _1);

      sendInterest(interest, onValidated, onValidationFailed, timeoutNotify, 0);
    }
  
}

void
ContactManager::onFetchIdCert(const QString& qCertName)
{
  Name certName(qCertName.toStdString());
  if(m_bufferedIdCerts.find(certName) != m_bufferedIdCerts.end())
    {
      emit idCertReady(*m_bufferedIdCerts[certName]);
    }
}

void
ContactManager::onAddFetchedContactIdCert(const QString& qCertName)
{
  Name certName(qCertName.toStdString());
  Name identity = IdentityCertificate::certificateNameToPublicKeyName(certName).getPrefix(-1);

  BufferedIdCerts::const_iterator it = m_bufferedIdCerts.find(certName);
  if(it != m_bufferedIdCerts.end())
    {
      Contact contact(*it->second);
      try
        {
          m_contactStorage->addContact(contact);
          m_bufferedIdCerts.erase(certName);

          m_contactList.clear();
          m_contactStorage->getAllContacts(m_contactList);

          onWaitForContactList();
        }
      catch(ContactStorage::Error& e)
        {
          emit warning(QString::fromStdString(e.what()));
        }
    }
  else
    emit warning(QString("Failure: no information of %1").arg(QString::fromStdString(identity.toUri())));
}

void
ContactManager::onWaitForContactList()
{
  ContactList::const_iterator it  = m_contactList.begin();
  ContactList::const_iterator end = m_contactList.end();
  
  QStringList aliasList;
  QStringList idList;
  for(; it != end; it++)
    {
      aliasList << QString((*it)->getAlias().c_str());
      idList << QString((*it)->getNameSpace().toUri().c_str());
    }
  
  emit contactAliasListReady(aliasList);
  emit contactIdListReady(idList);
}

void
ContactManager::onWaitForContactInfo(const QString& identity)
{
  ContactList::const_iterator it  = m_contactList.begin();
  ContactList::const_iterator end = m_contactList.end();

  for(; it != end; it++)
    if((*it)->getNameSpace().toUri() == identity.toStdString())
      emit contactInfoReady(QString((*it)->getNameSpace().toUri().c_str()),
                            QString((*it)->getName().c_str()),
                            QString((*it)->getInstitution().c_str()),
                            (*it)->isIntroducer());
}

void
ContactManager::onRemoveContact(const QString& identity)
{
  m_contactStorage->removeContact(Name(identity.toStdString()));
  m_contactList.clear();
  m_contactStorage->getAllContacts(m_contactList);

  onWaitForContactList();
}

void
ContactManager::onUpdateAlias(const QString& identity, const QString& alias)
{
  m_contactStorage->updateAlias(Name(identity.toStdString()), alias.toStdString());
  m_contactList.clear();
  m_contactStorage->getAllContacts(m_contactList);

  onWaitForContactList();
}

void
ContactManager::onUpdateIsIntroducer(const QString& identity, bool isIntroducer)
{
  m_contactStorage->updateIsIntroducer(Name(identity.toStdString()), isIntroducer);
}

void
ContactManager::onUpdateEndorseCertificate(const QString& identity)
{
  Name identityName(identity.toStdString());
  shared_ptr<EndorseCertificate> newEndorseCertificate = generateEndorseCertificate(identityName);

  if(!static_cast<bool>(newEndorseCertificate))
    return;

  m_contactStorage->addEndorseCertificate(*newEndorseCertificate, identityName);

  publishEndorseCertificateInDNS(*newEndorseCertificate);
}

} // namespace chronos


#if WAF
#include "contact-manager.moc"
#include "contact-manager.cpp.moc"
#endif
