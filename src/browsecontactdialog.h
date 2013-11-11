/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */


#ifndef BROWSECONTACTDIALOG_H
#define BROWSECONTACTDIALOG_H

#include <QDialog>
#include <QStringListModel>
#include <QCloseEvent>
#include "warningdialog.h"


#ifndef Q_MOC_RUN
#include <ndn.cxx/security/certificate/identity-certificate.h>
#include <boost/thread/locks.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include "profile.h"
#include "contact-manager.h"
#endif

namespace Ui {
class BrowseContactDialog;
}

class BrowseContactDialog : public QDialog
{
  Q_OBJECT
  
public:
  explicit BrowseContactDialog(ndn::Ptr<ContactManager> contactManager,
                               QWidget *parent = 0);

  ~BrowseContactDialog();

protected:
  typedef boost::recursive_mutex RecLock;
  typedef boost::unique_lock<RecLock> UniqueRecLock;

private:
  void
  getCertNames(std::vector<std::string> &names);

  void
  updateCertificateMap(bool filter = false);

  void
  updateCertificateFetchingStatus(int count);

  void
  fetchCertificate();

protected:
  void
  closeEvent(QCloseEvent *e);

private slots:
  void
  updateSelection(const QItemSelection &selected,
                  const QItemSelection &deselected);

  void
  onCertificateFetched(const ndn::security::IdentityCertificate& identityCertificate);

  void
  onCertificateFetchFailed(const ndn::Name& identity);

  void
  onWarning(QString msg);

  void
  onAddClicked();

  void
  onDirectAddClicked();

public slots:
  void
  refreshList();

signals:
  void
  newContactAdded();

  void
  directAddClicked();

private:
  Ui::BrowseContactDialog *ui;
  
  ndn::Ptr<ContactManager> m_contactManager;

  WarningDialog* m_warningDialog;
  QStringListModel* m_contactListModel;

  QStringList m_contactList;  
  std::vector<ndn::Name> m_contactNameList;
  std::vector<ndn::Name> m_certificateNameList;
  std::map<ndn::Name, ndn::security::IdentityCertificate> m_certificateMap;
  std::map<ndn::Name, Profile> m_profileMap;

  RecLock m_mutex;


};

#endif // BROWSECONTACTDIALOG_H
