TEMPLATE = app
TARGET = sync-demo
DEFINES += __DEBUG
HEADERS = chatdialog.h \
          digesttreescene.h \
          settingdialog.h \
          treelayout.h

SOURCES = main.cpp \
          chatdialog.cpp \
          digesttreescene.cpp \
          settingdialog.cpp \
          treelayout.cpp 

RESOURCES = demo.qrc
ICON = demo.icns
QT += xml svg
FORMS = chatdialog.ui \
        settingdialog.ui

QMAKE_CXXFLAGS *= -g 
QMAKE_CFLAGS *= -g 
LIBS += -lboost_system-mt -lboost_random-mt

CONFIG += console 

PROTOS = chatbuf.proto
include (sync-demo.pri)

CONFIG += link_pkgconfig 
PKGCONFIG += libsync
PKGCONFIG += protobuf
