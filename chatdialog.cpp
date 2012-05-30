#include <QtGui>
#include "chatdialog.h"

ChatDialog::ChatDialog(QWidget *parent)
  : QDialog(parent)
{
  setupUi(this);
  lineEdit->setFocusPolicy(Qt::StrongFocus);
}
