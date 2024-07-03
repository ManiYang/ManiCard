include(gtest_dependency.pri)

TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG += thread

QT -= GUI
QT += testlib


SOURCES += \
        ../../src/utilities/async_routine.cpp \
        ../../src/utilities/json_util.cpp \
        main.cpp         \
        utilities/async_routine_unittest.cpp \
        utilities/json_util_unittest.cpp


HEADERS += \
    ../../src/utilities/async_routine.h \
    ../../src/utilities/json_util.h


INCLUDEPATH += ../../src/
DEPENDPATH += ../../src/

DEFINES += QT_MESSAGELOGCONTEXT
