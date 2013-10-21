/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef WARNINGDIALOG_H
#define WARNINGDIALOG_H

#include <QDialog>

namespace Ui {
class WarningDialog;
}

class WarningDialog : public QDialog
{
  Q_OBJECT

public:
  explicit WarningDialog(QWidget *parent = 0);

  ~WarningDialog();
                  
  void
  setMsg(const std::string& msg);

private slots:
  void
  onOkClicked();

private:
  Ui::WarningDialog *ui;
};

#endif // WARNINGDIALOG_H
