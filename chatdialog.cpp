#include <QtGui>
#include "chatdialog.h"
#include "settingdialog.h"
#include <ctime>
#include <iostream>
#include <QTimer>

ChatDialog::ChatDialog(QWidget *parent)
  : QDialog(parent)
{
  setupUi(this);

  readSettings();
  updateLabels();

  lineEdit->setFocusPolicy(Qt::StrongFocus);
  DigestTreeScene *scene = new DigestTreeScene();

  treeViewer->setScene(scene);
  scene->plot();

  connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(returnPressed()));
  connect(setButton, SIGNAL(pressed()), this, SLOT(buttonPressed()));
}

void
ChatDialog::appendMessage(const SyncDemo::ChatMessage &msg) 
{

  if (msg.type() != SyncDemo::ChatMessage::CHAT) {
    return;
  }

  if (!msg.has_data()) {
    return;
  }

  if (msg.from().empty() || msg.data().empty()) {
    return;
  }

  QTextCursor cursor(textEdit->textCursor());
  cursor.movePosition(QTextCursor::End);
  QTextTableFormat tableFormat;
  tableFormat.setBorder(0);
  QTextTable *table = cursor.insertTable(1, 2, tableFormat);
  QString from = QString("<%1>: ").arg(msg.from().c_str());
  table->cellAt(0, 0).firstCursorPosition().insertText(from);
  table->cellAt(0, 1).firstCursorPosition().insertText(msg.data().c_str());
  QScrollBar *bar = textEdit->verticalScrollBar();
  bar->setValue(bar->maximum());
}

void
ChatDialog::formChatMessage(const QString &text, SyncDemo::ChatMessage &msg) {
  msg.set_from(m_nick.toStdString());
  msg.set_to(m_chatroom.toStdString());
  msg.set_data(text.toStdString());
  time_t seconds = time(NULL);
  msg.set_timestamp(seconds);
}

void
ChatDialog::readSettings()
{
  QSettings s(ORGANIZATION, APPLICATION);
  m_nick = s.value("nick", "").toString();
  m_chatroom = s.value("chatroom", "").toString();
  m_prefix = s.value("prefix", "").toString();
  if (m_nick == "" || m_chatroom == "" || m_prefix == "") {
    QTimer::singleShot(500, this, SLOT(buttonPressed()));
  }
}

void 
ChatDialog::writeSettings()
{
  QSettings s(ORGANIZATION, APPLICATION);
  s.setValue("nick", m_nick);
  s.setValue("chatroom", m_chatroom);
  s.setValue("prefix", m_prefix);
}

void
ChatDialog::updateLabels()
{
  QString settingDisp = QString("<User: %1>, <Chatroom: %2>").arg(m_nick).arg(m_chatroom);
  infoLabel->setText(settingDisp);
  QString prefixDisp = QString("<Prefix: %1>").arg(m_prefix);
  prefixLabel->setText(prefixDisp);
}

void 
ChatDialog::returnPressed()
{
  QString text = lineEdit->text();
  if (text.isEmpty())
    return;
 
  lineEdit->clear();

  SyncDemo::ChatMessage msg;
  formChatMessage(text, msg);
  
  // TODO:
  // send message
  appendMessage(msg);
  
}

void
ChatDialog::buttonPressed()
{
  SettingDialog dialog(this, m_nick, m_chatroom, m_prefix);
  connect(&dialog, SIGNAL(updated(QString, QString, QString)), this, SLOT(settingUpdated(QString, QString, QString)));
  dialog.exec();
}

void
ChatDialog::settingUpdated(QString nick, QString chatroom, QString prefix)
{
  bool needWrite = false;
  if (!nick.isEmpty() && nick != m_nick) {
    m_nick = nick;
    needWrite = true;
  }
  if (!prefix.isEmpty() && prefix != m_prefix) {
    m_prefix = prefix;
    needWrite = true;
    // TODO: set the previous prefix as left?
  }
  if (!chatroom.isEmpty() && chatroom != m_chatroom) {
    m_chatroom = chatroom;
    needWrite = true;
    // TODO: perhaps need to do a lot. e.g. use a new SyncAppSokcet
  }
  if (needWrite) {
    writeSettings();
    updateLabels();
  }
}
