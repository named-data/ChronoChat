/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Zhenkai Zhu <zhenkai@cs.ucla.edu>
 *         Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *         Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "chat-dialog.hpp"
#include "ui_chat-dialog.h"

#include <QScrollBar>
#include <QMessageBox>
#include <QCloseEvent>

Q_DECLARE_METATYPE(ndn::Name)
Q_DECLARE_METATYPE(time_t)
Q_DECLARE_METATYPE(std::vector<chronochat::NodeInfo>)
Q_DECLARE_METATYPE(uint64_t)

namespace chronochat {

static const Name PRIVATE_PREFIX("/private/local");
static const ndn::Name::Component ROUTING_HINT_SEPARATOR =
  ndn::name::Component::fromEscapedString("%F0%2E");

ChatDialog::ChatDialog(const Name& chatroomPrefix,
                       const Name& userChatPrefix,
                       const Name& routingPrefix,
                       const std::string& chatroomName,
                       const std::string& nick,
                       bool isSecured,
                       const Name& signingId,
                       QWidget* parent)
  : QDialog(parent)
  , ui(new Ui::ChatDialog)
  , m_backend(chatroomPrefix, userChatPrefix, routingPrefix, chatroomName, nick, signingId)
  , m_chatroomName(chatroomName)
  , m_chatroomPrefix(chatroomPrefix)
  , m_nick(nick.c_str())
  , m_isSecured(isSecured)
{
  qRegisterMetaType<ndn::Name>("ndn::Name");
  qRegisterMetaType<time_t>("time_t");
  qRegisterMetaType<std::vector<chronochat::NodeInfo> >("std::vector<chronochat::NodeInfo>");
  qRegisterMetaType<uint64_t>("uint64_t");

  m_scene = new DigestTreeScene(this);
  m_trustScene = new TrustTreeScene(this);
  m_rosterModel = new QStringListModel(this);

  ui->setupUi(this);

  ui->syncTreeViewer->setScene(m_scene);
  m_scene->setSceneRect(m_scene->itemsBoundingRect());
  ui->syncTreeViewer->hide();

  ui->trustTreeViewer->setScene(m_trustScene);
  m_trustScene->setSceneRect(m_trustScene->itemsBoundingRect());
  ui->trustTreeViewer->hide();

  ui->listView->setModel(m_rosterModel);

  Name routablePrefix;

  if (routingPrefix.isPrefixOf(userChatPrefix))
    routablePrefix = userChatPrefix;
  else
    routablePrefix.append(routingPrefix)
      .append(ROUTING_HINT_SEPARATOR)
      .append(userChatPrefix);

  updateLabels(routablePrefix);

  QStringList roster;
  roster << "- " + m_nick;
  m_rosterModel->setStringList(roster);

  ui->syncTreeButton->setText("Hide ChronoSync Tree");

  // When backend receives a sync update, notify frontend to update sync tree
  connect(&m_backend, SIGNAL(syncTreeUpdated(std::vector<chronochat::NodeInfo>, QString)),
          this,       SLOT(updateSyncTree(std::vector<chronochat::NodeInfo>, QString)));

  // When backend receives a new chat message, notify frontent to print it out.
  connect(&m_backend, SIGNAL(chatMessageReceived(QString, QString, time_t)),
          this,       SLOT(receiveChatMessage(QString, QString, time_t)));

  // When backend detects a deleted session, notify frontend to print the message.
  connect(&m_backend, SIGNAL(sessionRemoved(QString, QString, time_t)),
          this,       SLOT(removeSession(QString, QString, time_t)));

  // When backend receives a new message, notify frontend to print notification
  connect(&m_backend, SIGNAL(messageReceived(QString, QString, uint64_t, time_t, bool)),
          this,       SLOT(receiveMessage(QString, QString, uint64_t, time_t, bool)));

  // When backend updates prefix, notify frontend to update labels.
  connect(&m_backend, SIGNAL(chatPrefixChanged(ndn::Name)),
          this,       SLOT(updateLabels(ndn::Name)));

  connect(&m_backend, SIGNAL(refreshChatDialog(ndn::Name)),
          this,       SLOT(updateLabels(ndn::Name)));

  // When frontend gets a message to send, notify backend.
  connect(this,       SIGNAL(msgToSent(QString, time_t)),
          &m_backend, SLOT(sendChatMessage(QString, time_t)));

  // When frontend gets a shutdown command, notify backend.
  connect(this,       SIGNAL(shutdownBackend()),
          &m_backend, SLOT(shutdown()));

  m_scene->plot("Empty");

  connect(ui->lineEdit, SIGNAL(returnPressed()),
          this, SLOT(onReturnPressed()));
  connect(ui->syncTreeButton, SIGNAL(pressed()),
          this, SLOT(onSyncTreeButtonPressed()));
  connect(ui->trustTreeButton, SIGNAL(pressed()),
          this, SLOT(onTrustTreeButtonPressed()));

  disableSyncTreeDisplay();
  QTimer::singleShot(2200, this, SLOT(enableSyncTreeDisplay()));

  m_backend.start();
}


ChatDialog::~ChatDialog()
{
}

void
ChatDialog::closeEvent(QCloseEvent *e)
{
  // When close button is clicked, do not close the dialog immediately.

  QMessageBox::information(this, tr("ChronoChat"),
                           tr("The chatroom will keep running in the "
                              "system tray. To close the chatroom, "
                              "choose <b>Close chatroom</b> in the "
                              "context memu of the system tray entry."));
  hide();
  e->ignore();
}

void
ChatDialog::changeEvent(QEvent *e)
{
  switch(e->type()) {
  case QEvent::ActivationChange:
    if (isActiveWindow()) {
      emit resetIcon();
    }
    break;
  default:
    break;
  }
}

void
ChatDialog::resizeEvent(QResizeEvent *e)
{
  fitView();
}

void
ChatDialog::showEvent(QShowEvent *e)
{
  fitView();
}

ChatroomInfo
ChatDialog::getChatroomInfo()
{
  ChatroomInfo chatroomInfo;
  chatroomInfo.setName(Name::Component(m_chatroomName));
  QStringList prefixList = m_scene->getRosterPrefixList();
  for(QStringList::iterator it = prefixList.begin();
      it != prefixList.end(); ++it ) {
    Name participant = Name(it->toStdString()).getPrefix(-3);
    chatroomInfo.addParticipant(participant);
  }

  chatroomInfo.setSyncPrefix(m_chatroomPrefix);
  if (m_isSecured)
    chatroomInfo.setTrustModel(ChatroomInfo::TRUST_MODEL_HIERARCHICAL);
  else
    chatroomInfo.setTrustModel(ChatroomInfo::TRUST_MODEL_NONE);
  return chatroomInfo;
}

// private methods:
void ChatDialog::disableSyncTreeDisplay()
{
  ui->syncTreeButton->setEnabled(false);
  ui->syncTreeViewer->hide();
  fitView();
}

void
ChatDialog::appendChatMessage(const QString& nick, const QString& text, time_t timestamp)
{
  QTextCharFormat nickFormat;
  nickFormat.setForeground(Qt::darkGreen);
  nickFormat.setFontWeight(QFont::Bold);
  nickFormat.setFontUnderline(true);
  nickFormat.setUnderlineColor(Qt::gray);

  // Print who & when
  QTextCursor cursor(ui->textEdit->textCursor());
  cursor.movePosition(QTextCursor::End);
  QTextTableFormat tableFormat;
  tableFormat.setBorder(0);
  QTextTable *table = cursor.insertTable(1, 2, tableFormat);
  QString from = QString("%1 ").arg(nick);
  QTextTableCell fromCell = table->cellAt(0, 0);
  fromCell.setFormat(nickFormat);
  fromCell.firstCursorPosition().insertText(from);
  printTimeInCell(table, timestamp);

  // Print what
  QTextCursor nextCursor(ui->textEdit->textCursor());
  nextCursor.movePosition(QTextCursor::End);
  table = nextCursor.insertTable(1, 1, tableFormat);
  table->cellAt(0, 0).firstCursorPosition().insertText(text);

  // Popup notification
  showMessage(from, text);

  QScrollBar *bar = ui->textEdit->verticalScrollBar();
  bar->setValue(bar->maximum());
}

void
ChatDialog::appendControlMessage(const QString& nick,
                                 const QString& action,
                                 time_t timestamp)
{
  QTextCharFormat nickFormat;
  nickFormat.setForeground(Qt::gray);
  nickFormat.setFontWeight(QFont::Bold);
  nickFormat.setFontUnderline(true);
  nickFormat.setUnderlineColor(Qt::gray);

  QTextCursor cursor(ui->textEdit->textCursor());
  cursor.movePosition(QTextCursor::End);
  QTextTableFormat tableFormat;
  tableFormat.setBorder(0);
  QTextTable *table = cursor.insertTable(1, 2, tableFormat);

  QString controlMsg = QString("%1 %2  ").arg(nick).arg(action);
  QTextTableCell fromCell = table->cellAt(0, 0);
  fromCell.setFormat(nickFormat);
  fromCell.firstCursorPosition().insertText(controlMsg);
  printTimeInCell(table, timestamp);
}

QString
ChatDialog::formatTime(time_t timestamp)
{
  struct tm* localTime = localtime(&timestamp);

  return QString("%1:%2:%3")
           .arg(localTime->tm_hour, 2, 10, QChar('0'))
           .arg(localTime->tm_min, 2, 10, QChar('0'))
           .arg(localTime->tm_sec, 2, 10, QChar('0'));
}

void
ChatDialog::printTimeInCell(QTextTable *table, time_t timestamp)
{
  QTextCharFormat timeFormat;
  timeFormat.setForeground(Qt::gray);
  timeFormat.setFontUnderline(true);
  timeFormat.setUnderlineColor(Qt::gray);
  QTextTableCell timeCell = table->cellAt(0, 1);
  timeCell.setFormat(timeFormat);
  timeCell.firstCursorPosition().insertText(formatTime(timestamp));
}

void
ChatDialog::showMessage(const QString& from, const QString& data)
{
  if (!isActiveWindow())
    emit showChatMessage(QString::fromStdString(m_chatroomName),
                         from, data);
}

void
ChatDialog::fitView()
{
  QRectF rect = m_scene->itemsBoundingRect();
  m_scene->setSceneRect(rect);
  ui->syncTreeViewer->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);

  QRectF trustRect = m_trustScene->itemsBoundingRect();
  m_trustScene->setSceneRect(trustRect);
  ui->trustTreeViewer->fitInView(m_trustScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

// public slots:
void
ChatDialog::onShow()
{
  this->show();
  this->raise();
  this->activateWindow();
}

void
ChatDialog::shutdown()
{
  if (m_backend.isRunning()) {
    emit shutdownBackend();
    m_backend.wait();
  }
  hide();
  emit closeChatDialog(QString::fromStdString(m_chatroomName));
}

// private slots:
void
ChatDialog::updateSyncTree(std::vector<chronochat::NodeInfo> updates, QString rootDigest)
{
  m_scene->processSyncUpdate(updates, rootDigest);
}

void
ChatDialog::receiveChatMessage(QString nick, QString text, time_t timestamp)
{
  appendChatMessage(nick, text, timestamp);
}

void
ChatDialog::removeSession(QString sessionPrefix, QString nick, time_t timestamp)
{
  appendControlMessage(nick, "leaves room", timestamp);
  m_scene->removeNode(sessionPrefix);
  m_rosterModel->setStringList(m_scene->getRosterList());
  fitView();
}

void
ChatDialog::receiveMessage(QString sessionPrefix, QString nick, uint64_t seqNo, time_t timestamp,
                           bool addSession)
{
  m_scene->updateNode(sessionPrefix, nick, seqNo);
  m_scene->messageReceived(sessionPrefix);
  if (addSession) {
    appendControlMessage(nick, "enters room", timestamp);
    m_rosterModel->setStringList(m_scene->getRosterList());
  }
  fitView();
}

void
ChatDialog::updateLabels(Name newChatPrefix)
{
  // Reset DigestTree
  m_scene->clearAll();
  m_scene->plot("Empty");

  // Display chatroom name
  QString chatroomName = QString("Chatroom: %1").arg(QString::fromStdString(m_chatroomName));
  ui->infoLabel->setStyleSheet("QLabel {color: #630; font-size: 16px; font: bold \"Verdana\";}");
  ui->infoLabel->setText(chatroomName);

  // Display chat message prefix
  QString chatPrefix;
  if (PRIVATE_PREFIX.isPrefixOf(newChatPrefix)) {
    chatPrefix =
      QString("<Warning: no connection to hub or hub does not support prefix autoconfig.>\n"
              "<Prefix = %1>")
      .arg(QString::fromStdString(newChatPrefix.toUri()));
    ui->prefixLabel->setStyleSheet(
      "QLabel {color: red; font-size: 12px; font: bold \"Verdana\";}");
  }
  else {
    chatPrefix = QString("<Prefix = %1>")
      .arg(QString::fromStdString(newChatPrefix.toUri()));
    ui->prefixLabel->setStyleSheet(
      "QLabel {color: Green; font-size: 12px; font: bold \"Verdana\";}");
  }
  ui->prefixLabel->setText(chatPrefix);
  fitView();
}

void
ChatDialog::onReturnPressed()
{
  QString text = ui->lineEdit->text();

  if (text.isEmpty())
    return;

  ui->lineEdit->clear();

  time_t timestamp =
    static_cast<time_t>(time::toUnixTimestamp(time::system_clock::now()).count() / 1000);
  // appendChatMessage(m_nick, text, timestamp);

  emit msgToSent(text, timestamp);

  fitView();
}

void
ChatDialog::onSyncTreeButtonPressed()
{
  if (ui->syncTreeViewer->isVisible()) {
    ui->syncTreeViewer->hide();
    ui->syncTreeButton->setText("Show ChronoSync Tree");
  }
  else {
    ui->syncTreeViewer->show();
    ui->syncTreeButton->setText("Hide ChronoSync Tree");
  }

  fitView();
}

void
ChatDialog::onTrustTreeButtonPressed()
{
  if (ui->trustTreeViewer->isVisible()) {
    ui->trustTreeViewer->hide();
    ui->trustTreeButton->setText("Show Trust Tree");
  }
  else {
    ui->trustTreeViewer->show();
    ui->trustTreeButton->setText("Hide Trust Tree");
  }

  fitView();
}

void ChatDialog::enableSyncTreeDisplay()
{
  ui->syncTreeButton->setEnabled(true);
  ui->syncTreeViewer->show();
  fitView();
}

} // namespace chronochat


#if WAF
#include "chat-dialog.moc"
// #include "chat-dialog.cpp.moc"
#endif
