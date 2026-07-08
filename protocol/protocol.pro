TEMPLATE = lib
CONFIG += staticlib
TARGET = protocol

QT += core network

SOURCES += message.cpp
HEADERS += message.h

DESTDIR = $$PWD/../lib
