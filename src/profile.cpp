/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "profile.h"
#include <ndn.cxx/helpers/der/der.h>
#include <ndn.cxx/helpers/der/visitor/simple-visitor.h>

using namespace std;
using namespace ndn;

Profile::Profile(const Name& identityName)
  : m_identityName(identityName)
{
  const string& nameString = identityName.toUri();
  Blob identityBlob (nameString.c_str(), nameString.size());
  m_entries[string("IDENTITY")] = identityBlob;
}

Profile::Profile(const Name& identityName,
		 const string& name,
		 const string& institution)
  : m_identityName(identityName)
{
  const string& nameString = identityName.toUri();
  Blob identityBlob (nameString.c_str(), nameString.size());
  m_entries[string("IDENTITY")] = identityBlob;

  Blob nameBlob (name.c_str(), name.size());
  Blob institutionBlob (institution.c_str(), institution.size());

  m_entries[string("name")] = nameBlob;
  m_entries[string("institution")] = institutionBlob;
}

Profile::Profile(const Profile& profile)
  : m_identityName(profile.m_identityName)
  , m_entries(profile.m_entries)
{}

void
Profile::setProfileEntry(const string& profileType,
			 const Blob& profileValue)
{ m_entries[profileType] = profileValue; }

Ptr<const Blob>
Profile::getProfileEntry(const string& profileType) const
{
  if(m_entries.find(profileType) != m_entries.end())
    return Ptr<Blob>(new Blob(m_entries.at(profileType).buf(), m_entries.at(profileType).size()));

  return NULL;
}

Ptr<Blob>
Profile::toDerBlob() const
{
  Ptr<der::DerSequence> root = Ptr<der::DerSequence>::Create();
  
  Ptr<der::DerPrintableString> identityName = Ptr<der::DerPrintableString>(new der::DerPrintableString(m_identityName.toUri()));
  root->addChild(identityName);

  map<string, Blob>::const_iterator it = m_entries.begin();
  for(; it != m_entries.end(); it++)
    {
      Ptr<der::DerSequence> entry = Ptr<der::DerSequence>::Create();
      Ptr<der::DerPrintableString> type = Ptr<der::DerPrintableString>(new der::DerPrintableString(it->first));
      Ptr<der::DerOctetString> value = Ptr<der::DerOctetString>(new der::DerOctetString(it->second));
      entry->addChild(type);
      entry->addChild(value);
      root->addChild(entry);
    }
  
  blob_stream blobStream;
  OutputIterator & start = reinterpret_cast<OutputIterator &> (blobStream);
  root->encode(start);

  return blobStream.buf ();
}

Ptr<Profile>
Profile::fromDerBlob(const Blob& derBlob)
{
  boost::iostreams::stream
    <boost::iostreams::array_source> is (derBlob.buf(), derBlob.size());

  Ptr<der::DerSequence> root = DynamicCast<der::DerSequence>(der::DerNode::parse(reinterpret_cast<InputIterator &>(is)));
  
  const der::DerNodePtrList & children = root->getChildren();
  der::SimpleVisitor simpleVisitor;
  string identityName = boost::any_cast<string>(children[0]->accept(simpleVisitor));
  Ptr<Profile> profile = Ptr<Profile>(new Profile(identityName));

  for(int i = 1; i < children.size(); i++)
    {
      Ptr<der::DerSequence> entry = DynamicCast<der::DerSequence>(children[i]);
      const der::DerNodePtrList & tuple = root->getChildren();
      string type = boost::any_cast<string>(tuple[0]->accept(simpleVisitor));
      Ptr<Blob> value = boost::any_cast<Ptr<Blob> >(tuple[1]->accept(simpleVisitor));
      profile->setProfileEntry(type, *value);
    }

  return profile;
}
