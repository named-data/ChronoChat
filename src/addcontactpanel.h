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

class AddContactPanel : public QDialog
{
  Q_OBJECT

public:
  explicit AddContactPanel(ndn::Ptr<ContactManager> contactManager,
                           QWidget *parent = 0);

  ~AddContactPanel();

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

signals:
  void
  newContactAdded();

private:
  Ui::AddContactPanel *ui;
  ndn::Name m_searchIdentity;
  ndn::Ptr<ContactManager> m_contactManager;
  WarningDialog* m_warningDialog;
  ndn::Ptr<EndorseCertificate> m_currentEndorseCertificate;
};

#endif // ADDCONTACTPANEL_H
