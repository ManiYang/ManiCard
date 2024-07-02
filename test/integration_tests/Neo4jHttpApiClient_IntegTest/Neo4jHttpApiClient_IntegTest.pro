include(gtest_dependency.pri)

TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG += thread

QT -= gui
QT += testlib network

SOURCES += \
        ../../../src/neo4j_http_api_client.cpp \
        ../../../src/utilities/json_util.cpp \
        ../../../src/utilities/logging.cpp \
        main.cpp         \
        neo4j_http_api_client_integtest.cpp

HEADERS += \
    ../../../src/neo4j_http_api_client.h \
    ../../../src/utilities/json_util.h \
    ../../../src/utilities/logging.h \
    test_util.h


INCLUDEPATH += ../../../src/
DEPENDPATH += ../../../src/

DEFINES += QT_MESSAGELOGCONTEXT
