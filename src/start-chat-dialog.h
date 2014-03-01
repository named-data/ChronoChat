/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef START_CHAT_DIALOG_H
#define START_CHAT_DIALOG_H

#include <QDialog>

#ifndef Q_MOC_RUN
#include <string>
#endif

namespace Ui {
class StartChatDialog;
}

class StartChatDialog : public QDialog
{
  Q_OBJECT

public:
  explicit StartChatDialog(QWidget *parent = 0);

  ~StartChatDialog();

  void
  setChatroom(const std::string& chatroom);

signals:
  void
  startChatroom(const QString& chatroomName, bool secured);
    
private slots:
  void
  onOkClicked();
  
  void
  onCancelClicked();

private:
  Ui::StartChatDialog *ui;
};

#endif // START_CHAT_DIALOG_H
