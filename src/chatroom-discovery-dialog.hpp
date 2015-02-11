/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Mengjin Yan <jane.yan0129@gmail.com>
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */
#ifndef CHRONOCHAT_CHATROOM_DISCOVERY_DIALOG_HPP
#define CHRONOCHAT_CHATROOM_DISCOVERY_DIALOG_HPP

#include <QDialog>
#include <QModelIndex>
#include <QStandardItem>
#include <QHeaderView>
#include <QMessageBox>
#include <QDebug>
#include "chatroom-discovery-view-dialog.hpp"

#ifndef Q_MOC_RUN
#include "chatroom-info.hpp"
#endif

namespace Ui {
class ChatroomDiscoveryDialog;
}

namespace chronochat {

class ChatroomDiscoveryDialog : public QDialog
{
  Q_OBJECT

public:
  explicit
  ChatroomDiscoveryDialog(QWidget* parent = 0);

  ~ChatroomDiscoveryDialog();

  void
  updateChatroomList();

signals:
  void
  startChatroom(const QString& chatroomName, bool secured);

public slots:
  void
  onDiscoverChatroomChanged(const chronochat::ChatroomInfo& chatroom, bool isAdd);

private slots:
  void
  onCancelButtonClicked();

  void
  onJoinButtonClicked();

  void
  onViewButtonClicked();

  void
  onChatroomListViewDoubleClicked(const QModelIndex& index);

  void
  onChatroomListViewClicked(const QModelIndex& index);

private:

  typedef std::map<Name::Component, ChatroomInfo> Chatrooms;

  Ui::ChatroomDiscoveryDialog *ui;
  QStandardItemModel* m_standardItemModel;

  int m_selectedRow;

  Chatrooms m_chatrooms;



  ChatroomDiscoveryViewDialog* m_chatroomDiscoveryViewDialog;
};

} //namespace chronochat

#endif // CHRONOCHAT_CHATROOM_DISCOVERY_DIALOG_HPP
