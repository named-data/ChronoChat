/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */


#include "browsecontactdialog.h"
#include "ui_browsecontactdialog.h"

#ifndef Q_MOC_RUN
#include <boost/asio.hpp>
#include <boost/tokenizer.hpp>
#include "logging.h"
#include "exception.h"
#endif

using namespace std;
using namespace ndn;

INIT_LOGGER("BrowseContactDialog");

// Q_DECLARE_METATYPE(ndn::security::IdentityCertificate)

BrowseContactDialog::BrowseContactDialog(Ptr<ContactManager> contactManager,
					 QWidget *parent) 
  : QDialog(parent)
  , ui(new Ui::BrowseContactDialog)
  , m_contactManager(contactManager)
  , m_warningDialog(new WarningDialog)
  , m_contactListModel(new QStringListModel)
{
  // qRegisterMetaType<ndn::security::IdentityCertificate>("NDNIdentityCertificate");

  ui->setupUi(this);

  ui->ContactList->setModel(m_contactListModel);

  connect(ui->ContactList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          this, SLOT(updateSelection(const QItemSelection &, const QItemSelection &)));
  connect(&*m_contactManager, SIGNAL(contactCertificateFetched(const ndn::security::IdentityCertificate &)),
	  this, SLOT(onCertificateFetched(const ndn::security::IdentityCertificate &)));
  connect(&*m_contactManager, SIGNAL(contactCertificateFetchFailed(const ndn::Name&)),
          this, SLOT(onCertificateFetchFailed(const ndn::Name&)));
  connect(&*m_contactManager, SIGNAL(warning(QString)),
	  this, SLOT(onWarning(QString)));

  connect(ui->AddButton, SIGNAL(clicked()),
	  this, SLOT(onAddClicked()));
	  
  connect(ui->CancelButton, SIGNAL(clicked()),
	  this, SLOT(onCancelClicked()));
}

BrowseContactDialog::~BrowseContactDialog()
{
  delete ui;
}


vector<string> 
BrowseContactDialog::getCertName()
{
  vector<string> result;
  try{
    using namespace boost::asio::ip;
    tcp::iostream request_stream;
    request_stream.expires_from_now(boost::posix_time::milliseconds(3000));
    request_stream.connect("ndncert.named-data.net","80");
    if(!request_stream)
      {
	m_warningDialog->setMsg("Fail to fetch certificate directory! #1");
	m_warningDialog->show();
	return result;
      }
    request_stream << "GET /cert/list/ HTTP/1.0\r\n";
    request_stream << "Host: ndncert.named-data.net\r\n\r\n";
    request_stream.flush();

    string line1;
    std::getline(request_stream,line1);
    if (!request_stream)
      {
	m_warningDialog->setMsg("Fail to fetch certificate directory! #2");
	m_warningDialog->show();
	return result;
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
    	m_warningDialog->setMsg("Fail to fetch certificate directory! #3");
	m_warningDialog->show();
	return result;
      }
    if (status_code!=200)
      {
    	m_warningDialog->setMsg("Fail to fetch certificate directory! #4");
	m_warningDialog->show();
	return result;
      }
    vector<string> headers;
    std::string header;
    while (std::getline(request_stream, header) && header != "\r")
      headers.push_back(header);

    std::stringbuf buffer;
    std::ostream os (&buffer);

    os << request_stream.rdbuf();
    
    using boost::tokenizer;
    using boost::escaped_list_separator;

    tokenizer<escaped_list_separator<char> > certItems(buffer.str(), escaped_list_separator<char> ('\\', '\n', '"'));
    tokenizer<escaped_list_separator<char> >::iterator it = certItems.begin();
    try{
      while (it != certItems.end())
        {
	  result.push_back(*it);
          it++;
        }
    }catch (exception &e){
      m_warningDialog->setMsg("Fail to fetch certificate directory! #5");
      m_warningDialog->show();
      return vector<string>();
    }
    result.pop_back();
    
    return result;
  }catch(std::exception &e){
    m_warningDialog->setMsg("Fail to fetch certificate directory! #N");
    m_warningDialog->show();
    return result;
  }
}

void
BrowseContactDialog::updateCertificateMap(bool filter)
{
  vector<string> certNameList = getCertName();
  if(filter)
    {
      map<Name, Name> certificateMap;

      vector<string>::iterator it = certNameList.begin();
  
      for(; it != certNameList.end(); it++)
	{
	  Name newCertName(*it);
	  Name keyName = security::IdentityCertificate::certificateNameToPublicKeyName(newCertName, true);
	  Name identity = keyName.getPrefix(keyName.size()-1);
	  
	  map<Name, Name>::iterator map_it = certificateMap.find(identity);
	  if(map_it != certificateMap.end())
	    {
	      Name oldCertName = map_it->second;
	      Name oldKeyName = security::IdentityCertificate::certificateNameToPublicKeyName(oldCertName, true);
	      if(keyName > oldKeyName)
		map_it->second = newCertName;
	      else if(keyName == oldKeyName && newCertName > oldCertName)
		map_it->second = newCertName;
	    }
	  else
	    {
	      certificateMap.insert(pair<Name, Name>(identity, newCertName));
	    }
	}
      map<Name, Name>::iterator map_it = certificateMap.begin();
      for(; map_it != certificateMap.end(); map_it++)
	m_certificateNameList.push_back(map_it->second);
    }
  else
    {
      vector<string>::iterator it = certNameList.begin();
  
      for(; it != certNameList.end(); it++)
	{
	  m_certificateNameList.push_back(*it);
	}
    }
}

void
BrowseContactDialog::fetchCertificate()
{
  vector<Name>::iterator it = m_certificateNameList.begin();
  int count = 0;
  for(; it != m_certificateNameList.end(); it++)
    {
      m_contactManager->fetchIdCertificate(*it);
    }
}

void
BrowseContactDialog::onCertificateFetched(const security::IdentityCertificate& identityCertificate)
{
  Name certName = identityCertificate.getName();
  Name certNameNoVersion = certName.getPrefix(certName.size()-1);
  m_certificateMap.insert(pair<Name, security::IdentityCertificate>(certNameNoVersion, identityCertificate));
  m_profileMap.insert(pair<Name, Profile>(certNameNoVersion, Profile(identityCertificate)));
  string name(m_profileMap[certNameNoVersion].getProfileEntry("name")->buf(), m_profileMap[certNameNoVersion].getProfileEntry("name")->size());
  // Name contactName = m_profileMap[certNameNoVersion].getIdentityName();
  {
      UniqueRecLock lock(m_mutex);
      m_contactList << QString::fromStdString(name);
      m_contactListModel->setStringList(m_contactList);
      m_contactNameList.push_back(certNameNoVersion);
  }    
}

void
BrowseContactDialog::onCertificateFetchFailed(const Name& identity)
{}

void
BrowseContactDialog::refreshList()
{
  {
      UniqueRecLock lock(m_mutex);
      m_contactList.clear();
      m_contactNameList.clear();
  }
  m_certificateNameList.clear();
  m_certificateMap.clear();
  m_profileMap.clear();

  updateCertificateMap();

  fetchCertificate();
}

void
BrowseContactDialog::updateSelection(const QItemSelection &selected,
				     const QItemSelection &deselected)
{
  QModelIndexList items = selected.indexes();
  Name certName = m_contactNameList[items.first().row()];

  ui->InfoTable->clear();
  for(int i = ui->InfoTable->rowCount() - 1; i >= 0 ; i--)
    ui->InfoTable->removeRow(i);

  map<Name, Profile>::iterator it = m_profileMap.find(certName);
  if(it != m_profileMap.end())
    {
      ui->InfoTable->setColumnCount(2);

      QTableWidgetItem *typeHeader = new QTableWidgetItem(QString::fromUtf8("Type"));
      ui->InfoTable->setHorizontalHeaderItem(0, typeHeader);
      QTableWidgetItem *valueHeader = new QTableWidgetItem(QString::fromUtf8("Value"));
      ui->InfoTable->setHorizontalHeaderItem(1, valueHeader);

      Profile::const_iterator pro_it = it->second.begin();
      int rowCount = 0;        
  
      for(; pro_it != it->second.end(); pro_it++, rowCount++)
	{
	  ui->InfoTable->insertRow(rowCount);  
	  QTableWidgetItem *type = new QTableWidgetItem(QString::fromStdString(pro_it->first));
	  ui->InfoTable->setItem(rowCount, 0, type);
	  
	  string valueString(pro_it->second.buf(), pro_it->second.size());
	  QTableWidgetItem *value = new QTableWidgetItem(QString::fromStdString(valueString));
	  ui->InfoTable->setItem(rowCount, 1, value);	  
	}
    }
}

void
BrowseContactDialog::onWarning(QString msg)
{
  m_warningDialog->setMsg(msg.toStdString());
  m_warningDialog->show();
}

void
BrowseContactDialog::onAddClicked()
{
  QItemSelectionModel* selectionModel = ui->ContactList->selectionModel();
  QModelIndexList selectedList = selectionModel->selectedIndexes();
  QModelIndexList::iterator it = selectedList.begin();
  for(; it != selectedList.end(); it++)
    {
      Name certName = m_contactNameList[it->row()];
      if(m_certificateMap.find(certName) != m_certificateMap.end() && m_profileMap.find(certName) != m_profileMap.end())
	m_contactManager->addContact(m_certificateMap[certName], m_profileMap[certName]);
      else
	{
	  m_warningDialog->setMsg("Not enough information to add contact!");
	  m_warningDialog->show();
	}
    }
  emit newContactAdded();
  this->close();
}

void
BrowseContactDialog::onCancelClicked()
{ this->close(); }

#if WAF
#include "browsecontactdialog.moc"
#include "browsecontactdialog.cpp.moc"
#endif
