#include "settingdialog.h"
#include <QRegExp>
#include <QValidator>

SettingDialog::SettingDialog(QWidget *parent, QString nick, QString chatroom, QString prefix)
  : QDialog(parent)
{
  setupUi(this);

  QRegExp noWhiteSpace("^\\S+.*$");
  QValidator *nwsValidator = new QRegExpValidator(noWhiteSpace, this);
  nickEdit->setPlaceholderText(nick);
  nickEdit->setValidator(nwsValidator);
  roomEdit->setPlaceholderText(chatroom);
  roomEdit->setValidator(nwsValidator);
  prefixEdit->setPlaceholderText(prefix);

  // simple validator for ccnx prefix
  QRegExp rx("(^(/[^/]+)+$)|(^/$)");
  QValidator *validator = new QRegExpValidator(rx, this);
  prefixEdit->setValidator(validator);

  if (nick.isEmpty())
  {
    prefixEdit->hide();
    prefixLabel->hide();
  }

  okButton->setDefault(true);

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

#if WAF
#include "settingdialog.moc"
#include "settingdialog.cpp.moc"
#endif
