#include <QtGui>
#include "chatdialog.h"
#include "settingdialog.h"
#include <ctime>
#include <iostream>

ChatDialog::ChatDialog(QWidget *parent)
  : QDialog(parent)
{
  setupUi(this);
  lineEdit->setFocusPolicy(Qt::StrongFocus);

  // for test only
  m_nick = "Tester";
  m_chatroom = "Test";
  m_prefix = "/ndn/ucla.edu/cs/tester";

  QString settingDisp = QString("<User: %1>, <Chatroom: %2>").arg(m_nick).arg(m_chatroom);
  infoLabel->setText(settingDisp);
  QString prefixDisp = QString("<Prefix: %1>").arg(m_prefix);
  prefixLabel->setText(prefixDisp);

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
  dialog.exec();
  setButton->setFocusPolicy(Qt::NoFocus);
}

