/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef ADD_CONTACT_PANEL_H
#define ADD_CONTACT_PANEL_H

#include <QDialog>
#include <QTableWidgetItem>

#ifndef Q_MOC_RUN
#include "endorse-info.pb.h"
#endif

namespace Ui {
class AddContactPanel;
}

class AddContactPanel : public QDialog
{
  Q_OBJECT

public:
  explicit
  AddContactPanel(QWidget *parent = 0);

  ~AddContactPanel();

public slots:
  void
  onContactEndorseInfoReady(const chronos::EndorseInfo& endorseInfo);

private slots:
  void
  onCancelClicked();

  void
  onSearchClicked();

  void
  onAddClicked();

signals:
  void
  fetchInfo(const QString& identity);

  void
  addContact(const QString& identity);

private:
  Ui::AddContactPanel *ui;
  QString m_searchIdentity;

  QTableWidgetItem* m_typeHeader;
  QTableWidgetItem* m_valueHeader;
  QTableWidgetItem* m_endorseHeader;
};

#endif // ADD_CONTACT_PANEL_H
