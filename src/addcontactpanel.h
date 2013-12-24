/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef ADDCONTACTPANEL_H
#define ADDCONTACTPANEL_H

#include <QDialog>
#include <QMetaType>
#include "warningdialog.h"

#ifndef Q_MOC_RUN
#include "endorse-certificate.h"
#include "profile.h"
#include "contact-manager.h"
#endif

namespace Ui {
class AddContactPanel;
}

Q_DECLARE_METATYPE(ndn::Name)

Q_DECLARE_METATYPE(EndorseCertificate)

Q_DECLARE_METATYPE(ndn::Data)

class AddContactPanel : public QDialog
{
  Q_OBJECT

public:
  explicit AddContactPanel(ndn::ptr_lib::shared_ptr<ContactManager> contactManager,
                           QWidget *parent = 0);

  ~AddContactPanel();

private:
  void
  displayContactInfo();

  bool
  isCorrectName(const ndn::Name& name);

  static bool
  isSameBlob(const ndn::Blob& blobA, const ndn::Blob& blobB);

private slots:
  void
  onCancelClicked();
  
  void
  onSearchClicked();

  void
  onAddClicked();

  void
  selfEndorseCertificateFetched(const EndorseCertificate& endorseCertificate);

  void
  selfEndorseCertificateFetchFailed(const ndn::Name& identity);

  void
  onContactKeyFetched(const EndorseCertificate& endorseCertificate);

  void
  onContactKeyFetchFailed(const ndn::Name& identity);

  void
  onCollectEndorseFetched(const ndn::Data& data);

  void
  onCollectEndorseFetchFailed(const ndn::Name& identity);

signals:
  void
  newContactAdded();

private:
  Ui::AddContactPanel *ui;
  ndn::Name m_searchIdentity;
  ndn::ptr_lib::shared_ptr<ContactManager> m_contactManager;
  WarningDialog* m_warningDialog;
  ndn::ptr_lib::shared_ptr<EndorseCertificate> m_currentEndorseCertificate;
  ndn::ptr_lib::shared_ptr<ndn::Data> m_currentCollectEndorse;
  bool m_currentEndorseCertificateReady;
  bool m_currentCollectEndorseReady;
};

#endif // ADDCONTACTPANEL_H
