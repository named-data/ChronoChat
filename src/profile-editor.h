/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef PROFILE_EDITOR_H
#define PROFILE_EDITOR_H

#include <QDialog>
#include <QtSql/QSqlTableModel>

#ifndef Q_MOC_RUN
#endif

namespace Ui {
class ProfileEditor;
}

class ProfileEditor : public QDialog
{
    Q_OBJECT

public:
  explicit ProfileEditor(QWidget *parent = 0);
  
  ~ProfileEditor();

public slots:
  void
  onCloseDBModule();

  void
  onIdentityUpdated(const QString& identity);
  
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
  Ui::ProfileEditor *ui;
  QSqlTableModel* m_tableModel;
  QString m_identity;
};

#endif // PROFILE_EDITOR_H
