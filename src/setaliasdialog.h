/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef SETALIASDIALOG_H
#define SETALIASDIALOG_H

#include <QDialog>

#ifndef Q_MOC_RUN
#include "contact-manager.h"
#endif

namespace Ui {
class SetAliasDialog;
}

class SetAliasDialog : public QDialog
{
  Q_OBJECT

public:
  explicit SetAliasDialog(ndn::ptr_lib::shared_ptr<ContactManager> contactManager,
			  QWidget *parent = 0);
  ~SetAliasDialog();

  void
  setTargetIdentity(const std::string& name);

signals:
  void
  aliasChanged();

private slots:
  void
  onOkClicked();

  void 
  onCancelClicked();

private:
  Ui::SetAliasDialog *ui;
  ndn::ptr_lib::shared_ptr<ContactManager> m_contactManager;
  std::string m_target;
};

#endif // SETALIASDIALOG_H
