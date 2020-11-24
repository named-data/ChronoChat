/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2020, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_PROFILE_EDITOR_HPP
#define CHRONOCHAT_PROFILE_EDITOR_HPP

#include <QDialog>
#include <QtSql/QSqlTableModel>

namespace Ui {
class ProfileEditor;
} // namespace Ui

namespace chronochat {

class ProfileEditor : public QDialog
{
    Q_OBJECT

public:
  explicit ProfileEditor(QWidget* parent = 0);

  ~ProfileEditor();

public slots:
  void
  onCloseDBModule();

  void
  onIdentityUpdated(const QString& identity);

  void
  resetPanel();

private slots:
  void
  onAddClicked();

  void
  onDeleteClicked();

  void
  onOkClicked();

signals:
  void
  updateProfile();

private:
  Ui::ProfileEditor* ui;
  QSqlTableModel* m_tableModel = 0;
  QString m_identity;
};

} // namespace chronochat

#endif // CHRONOCHAT_PROFILE_EDITOR_HPP
