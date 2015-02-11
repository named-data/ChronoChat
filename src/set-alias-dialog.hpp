/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_SET_ALIAS_DIALOG_HPP
#define CHRONOCHAT_SET_ALIAS_DIALOG_HPP

#include <QDialog>

#ifndef Q_MOC_RUN
#include <ndn-cxx/name.hpp>
#endif

namespace Ui {
class SetAliasDialog;
}

namespace chronochat {

class SetAliasDialog : public QDialog
{
  Q_OBJECT

public:
  explicit
  SetAliasDialog(QWidget* parent = 0);

  ~SetAliasDialog();

  void
  setTargetIdentity(const QString& targetIdentity, const QString& alias);

signals:
  void
  aliasChanged(const QString& identity, const QString& alias);

private slots:
  void
  onOkClicked();

  void
  onCancelClicked();

private:
  Ui::SetAliasDialog* ui;
  QString m_targetIdentity;
};

} // namespace chronochat

#endif // CHRONOCHAT_SET_ALIAS_DIALOG_HPP
