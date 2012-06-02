#include "settingdialog.h"
#include <QRegExp>
#include <QValidator>

SettingDialog::SettingDialog(QWidget *parent, QString nick, QString chatroom, QString prefix)
  : QDialog(parent)
{
  setupUi(this);
  nickEdit->setPlaceholderText(nick);
  roomEdit->setPlaceholderText(chatroom);
  prefixEdit->setPlaceholderText(prefix);

  // simple validator for ccnx prefix
  QRegExp rx("(^(/[^/]+)+$)|(^/$)");
  QValidator *validator = new QRegExpValidator(rx, this);
  prefixEdit->setValidator(validator);

  cancelButton->setDefault(true);

  connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
  connect(okButton, SIGNAL(clicked()), this, SLOT(update()));
}

void
SettingDialog::update() 
{
  emit updated(nickEdit->text(), roomEdit->text(), prefixEdit->text()); 
  accept();
}

void
SettingDialog::keyPressEvent(QKeyEvent *e)
{
  switch(e->key()) {
    case Qt::Key_Enter:
      update();
      break;
    default:
      QDialog::keyPressEvent(e);
  }
}
