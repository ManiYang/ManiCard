QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    cached_data_access.cpp \
    db_access/abstract_boards_data_access.cpp \
    db_access/abstract_cards_data_access.cpp \
    db_access/boards_data_access.cpp \
    db_access/cards_data_access.cpp \
    db_access/queued_db_access.cpp \
    file_access/unsaved_update_records_file.cpp \
    main.cpp \
    models/board.cpp \
    models/card.cpp \
    models/relationship.cpp \
    neo4j_http_api_client.cpp \
    services.cpp \
    utilities/app_instances_shared_memory.cpp \
    utilities/async_routine.cpp \
    utilities/json_util.cpp \
    utilities/logging.cpp \
    utilities/saving_debouncer.cpp \
    widgets/board_view.cpp \
    widgets/components/custom_graphics_text_item.cpp \
    widgets/components/custom_text_edit.cpp \
    widgets/components/graphics_item_move_resize.cpp \
    widgets/components/graphics_scene.cpp \
    widgets/components/node_rect.cpp \
    widgets/main_window.cpp

HEADERS += \
    cached_data_access.h \
    db_access/abstract_boards_data_access.h \
    db_access/abstract_cards_data_access.h \
    db_access/boards_data_access.h \
    db_access/cards_data_access.h \
    db_access/queued_db_access.h \
    file_access/unsaved_update_records_file.h \
    global_constants.h \
    models/board.h \
    models/card.h \
    models/node_labels.h \
    models/relationship.h \
    neo4j_http_api_client.h \
    services.h \
    utilities/app_instances_shared_memory.h \
    utilities/async_routine.h \
    utilities/functor.h \
    utilities/hash.h \
    utilities/json_util.h \
    utilities/logging.h \
    utilities/maps_util.h \
    utilities/margins_util.h \
    utilities/numbers_util.h \
    utilities/saving_debouncer.h \
    widgets/board_view.h \
    widgets/components/custom_graphics_text_item.h \
    widgets/components/custom_text_edit.h \
    widgets/components/graphics_item_move_resize.h \
    widgets/components/graphics_scene.h \
    widgets/components/node_rect.h \
    widgets/main_window.h

FORMS += \
    widgets/main_window.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


DEFINES += QT_MESSAGELOGCONTEXT

RESOURCES += \
    resources.qrc
