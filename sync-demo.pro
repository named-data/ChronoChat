TEMPLATE = app
TARGET = sync-demo
HEADERS = chatdialog.h \
          digesttreeviewer.h \
          digesttreescene.h

SOURCES = main.cpp \
          chatdialog.cpp \
          digesttreeviewer.cpp \
          digesttreescene.cpp

FORMS = chatdialog.ui

QMAKE_CXXFLAGS *= -g

QMAKE_LIBDIR *= /opt/local/lib /usr/local/lib /usr/lib ../../../third_party/OGDF/_release
INCLUDEPATH *= /opt/local/include /usr/local/include ../../../third_party/OGDF
LIBS *= -lccn -lssl -lcrypto -lpthread -lOGDF
CONFIG += console 

