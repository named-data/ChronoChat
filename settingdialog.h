#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H
#include "ui_settingdialog.h"

class SettingDialog : public QDialog, private Ui::SettingDialog
{
  Q_OBJECT

public:
  SettingDialog(QWidget *parent = 0, QString nick = QString("NULL"), QString chatroom = QString("NULL"), QString prefix = QString("NULL"));
};

#endif
