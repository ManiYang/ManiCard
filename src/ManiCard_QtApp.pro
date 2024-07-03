QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    db_access/abstract_cards_data_access.cpp \
    db_access/cards_data_access.cpp \
    main.cpp \
    main_window.cpp \
    models/board.cpp \
    models/card.cpp \
    models/relationship.cpp \
    neo4j_http_api_client.cpp \
    utilities/async_routine.cpp \
    utilities/json_util.cpp \
    utilities/logging.cpp

HEADERS += \
    db_access/abstract_cards_data_access.h \
    db_access/cards_data_access.h \
    global_constants.h \
    main_window.h \
    models/board.h \
    models/card.h \
    models/node_labels.h \
    models/relationship.h \
    neo4j_http_api_client.h \
    utilities/async_routine.h \
    utilities/functor.h \
    utilities/hash.h \
    utilities/json_util.h \
    utilities/logging.h

FORMS += \
    main_windowdow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


DEFINES += QT_MESSAGELOGCONTEXT
