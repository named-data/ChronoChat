/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef PROFILEEDITOR_H
#define PROFILEEDITOR_H

#include <QDialog>
#include <QtSql/QSqlTableModel>

#ifndef Q_MOC_RUN
#include "contact-storage.h"
#endif

namespace Ui {
class ProfileEditor;
}

class ProfileEditor : public QDialog
{
    Q_OBJECT

public:
  explicit ProfileEditor(ndn::Ptr<ContactStorage> contactStorage, QWidget *parent = 0);
  
  ~ProfileEditor();
  
private slots:
  void
  onAddClicked();

  void
  onDeleteClicked();

  void
  onOkClicked();


private:
  Ui::ProfileEditor *ui;
  QSqlTableModel* m_tableModel;
  ndn::Ptr<ContactStorage> m_contactStorage;
};

#endif // PROFILEEDITOR_H
