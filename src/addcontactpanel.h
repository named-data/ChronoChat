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

#ifndef Q_MOC_RUN
#include "endorse-certificate.h"
#endif

namespace Ui {
class AddContactPanel;
}

class AddContactPanel : public QDialog
{
    Q_OBJECT

public:
    explicit AddContactPanel(QWidget *parent = 0);
    ~AddContactPanel();

private slots:
  void
  onCancelClicked();
  
  void
  onSearchClicked();

  void
  onAddClicked();

private:
  Ui::AddContactPanel *ui;
  ndn::Ptr<EndorseCertificate> m_selfEndorseCertificate;
};

#endif // ADDCONTACTPANEL_H
