QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    main_window.cpp \
    neo4j_http_api_client.cpp \
    utilities/async_routine.cpp \
    utilities/async_task.cpp \
    utilities/json_util.cpp \
    utilities/logging.cpp

HEADERS += \
    global_constants.h \
    main_window.h \
    neo4j_http_api_client.h \
    utilities/async_routine.h \
    utilities/async_task.h \
    utilities/functor.h \
    utilities/json_util.h \
    utilities/logging.h

FORMS += \
    main_windowdow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


DEFINES += QT_MESSAGELOGCONTEXT
