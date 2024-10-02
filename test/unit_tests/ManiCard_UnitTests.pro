include(gtest_dependency.pri)

TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG += thread

QT -= GUI
QT += testlib


SOURCES += \
        ../../src/utilities/action_debouncer.cpp \
        ../../src/utilities/async_routine.cpp \
        ../../src/utilities/directed_graph.cpp \
        ../../src/utilities/json_util.cpp \
        main.cpp         \
        utilities/action_debouncer_unittest.cpp \
        utilities/async_routine_unittest.cpp \
        utilities/async_routine_with_error_flag_unittest.cpp \
        utilities/directed_graph_unittest.cpp \
        utilities/json_util_unittest.cpp \
        utilities/variables_update_propagator_unittest.cpp


HEADERS += \
    ../../src/utilities/action_debouncer.h \
    ../../src/utilities/async_routine.h \
    ../../src/utilities/directed_graph.h \
    ../../src/utilities/json_util.h \
    ../../src/utilities/variables_update_propagator.h


INCLUDEPATH += ../../src/
DEPENDPATH += ../../src/

DEFINES += QT_MESSAGELOGCONTEXT

include(boost_dependency.pri)
