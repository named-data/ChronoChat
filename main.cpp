#include<QApplication>
#include "chatdialog.h"

int main(int argc, char *argv[]) 
{
  QApplication app(argc, argv);
  
  ChatDialog dialog;
  dialog.show();
  dialog.activateWindow();
  return app.exec();
}
