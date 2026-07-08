TEMPLATE = app
TARGET = server

QT += core network sql

SOURCES += main.cpp compilerserver.cpp
HEADERS += compilerserver.h

INCLUDEPATH += $$PWD/../protocol
LIBS += -L$$PWD/../lib -lprotocol

DESTDIR = $$PWD/../bin
