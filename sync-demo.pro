TEMPLATE = app
TARGET = sync-demo
HEADERS = chatdialog.h 

SOURCES = main.cpp \
          chatdialog.cpp

FORMS = chatdialog.ui


QMAKE_LIBDIR *= /opt/local/lib /usr/local/lib /usr/lib
INCLUDEPATH *= /opt/local/include /usr/local/include
LIBS *= -lccn -lssl -lcrypto 
CONFIG += console 

