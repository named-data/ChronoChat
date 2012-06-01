TEMPLATE = app
TARGET = sync-demo
HEADERS = chatdialog.h \
          digesttreescene.h

SOURCES = main.cpp \
          chatdialog.cpp \
          digesttreescene.cpp

FORMS = chatdialog.ui

QMAKE_CXXFLAGS *= -g 
QMAKE_CFLAGS *= -g 

QMAKE_LIBDIR *= /opt/local/lib /usr/local/lib /usr/lib ../../../third_party/OGDF/_release ../build
INCLUDEPATH *= /opt/local/include /usr/local/include ../../../third_party/OGDF ../include
LIBS *= -lccn -lssl -lcrypto -lpthread -lOGDF -lprotobuf -lsync
CONFIG += console 

PROTOS = chatbuf.proto
include (sync-demo.pri)
