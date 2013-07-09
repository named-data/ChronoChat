#include<QApplication>
#include "chatdialog.h"
#include <QMessageBox>

int main(int argc, char *argv[])
{
  // Q_INIT_RESOURCE(demo);
  QApplication app(argc, argv);

  if (!QSystemTrayIcon::isSystemTrayAvailable()) {
    QMessageBox::critical(0, QObject::tr("Systray"),
			  QObject::tr("I couldn't detect any system tray "
				      "on this system."));
    return 1;
  }
  QApplication::setQuitOnLastWindowClosed (false);

#ifdef __APPLE__
	app.setWindowIcon(QIcon(":/demo.icns"));
#else
	app.setWindowIcon(QIcon(":/images/icon_large.png"));
#endif

  ChatDialog dialog;

  dialog.show ();
  dialog.activateWindow ();
  dialog.raise ();

  return app.exec();
}
