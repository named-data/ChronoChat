#include <QtGui>
#include "chatdialog.h"
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

  connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(returnPressed()));
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
ChatDialog::updateTreeView() 
{
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
  updateTreeView();
  
}

void
ChatDialog::formChatMessage(const QString &text, SyncDemo::ChatMessage &msg) {
  msg.set_from(m_nick.toStdString());
  msg.set_to(m_chatroom.toStdString());
  msg.set_data(text.toStdString());
  time_t seconds = time(NULL);
  msg.set_timestamp(seconds);
}
