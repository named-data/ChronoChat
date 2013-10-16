/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include <QApplication>
#include <QSystemTrayIcon>

#include "chronochat.h"
#include "contactpanel.h"
#include "contact-storage.h"
#include <ndn.cxx/security/identity/identity-manager.h>
#include <ndn.cxx/security/identity/osx-privatekey-storage.h>
#include <ndn.cxx/security/identity/basic-identity-storage.h>

using namespace ndn;

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);


//   app.setWindowIcon(QIcon("/Users/yuyingdi/Develop/QT/demo.icns"));
// // #else
// // 	app.setWindowIcon(QIcon("/Users/yuyingdi/Develop/QT/images/icon_large.png"));
// // #endif

  Ptr<security::BasicIdentityStorage> publicStorage = Ptr<security::BasicIdentityStorage>::Create();
  Ptr<security::OSXPrivatekeyStorage> privateStorage = Ptr<security::OSXPrivatekeyStorage>::Create();
  Ptr<security::IdentityManager> identityManager = Ptr<security::IdentityManager>(new security::IdentityManager(publicStorage, privateStorage));
  Ptr<ContactStorage> contactStorage = Ptr<ContactStorage>(new ContactStorage(identityManager));
  ContactPanel contactPanel(contactStorage);

  contactPanel.show ();
  contactPanel.activateWindow ();
  contactPanel.raise ();
  
  return app.exec();
}
