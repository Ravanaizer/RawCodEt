TEMPLATE = app
TARGET = client

QT += widgets

QT += webenginewidgets

QT += core gui network

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h


INCLUDEPATH += $$PWD/../protocol
LIBS += -L$$PWD/../lib -lprotocol

DESTDIR = $$PWD/../bin
