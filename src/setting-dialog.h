/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef SETTING_DIALOG_H
#define SETTING_DIALOG_H

#include <QDialog>

#ifndef Q_MOC_RUN
#endif

namespace Ui {
class SettingDialog;
}

class SettingDialog : public QDialog
{
  Q_OBJECT

public:
  explicit 
  SettingDialog(QWidget *parent = 0);

  ~SettingDialog();

  void
  setNick(const QString& nick);

signals:
  void
  identityUpdated(const QString& identity);

  void
  nickUpdated(const QString& nick);

public slots:
  void
  onIdentityUpdated(const QString& identity);

private slots:
  void
  onSaveClicked();

  void
  onCancelClicked();

private:
  Ui::SettingDialog *ui;
  QString m_identity;
  QString m_nick;
};

#endif // SETTING_DIALOG_H
