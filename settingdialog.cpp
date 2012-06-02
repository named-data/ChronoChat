#include "settingdialog.h"

SettingDialog::SettingDialog(QWidget *parent, QString nick, QString chatroom, QString prefix)
  : QDialog(parent)
{
  setupUi(this);
  nickEdit->setPlaceholderText(nick);
  roomEdit->setPlaceholderText(chatroom);
  prefixEdit->setPlaceholderText(prefix);
  connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
  connect(okButton, SIGNAL(clicked()), this, SLOT(update()));
}

void
SettingDialog::update() 
{
  emit updated(nickEdit->text(), roomEdit->text(), prefixEdit->text()); 
  accept();
}
