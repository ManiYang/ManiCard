QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    app_data.cpp \
    db_access/abstract_boards_data_access.cpp \
    db_access/abstract_cards_data_access.cpp \
    db_access/boards_data_access.cpp \
    db_access/cards_data_access.cpp \
    db_access/queued_db_access.cpp \
    file_access/local_settings_file.cpp \
    file_access/unsaved_update_records_file.cpp \
    main.cpp \
    models/board.cpp \
    models/boards_list_properties.cpp \
    models/card.cpp \
    models/node_rect_data.cpp \
    models/relationship.cpp \
    neo4j_http_api_client.cpp \
    persisted_data_access.cpp \
    services.cpp \
    utilities/action_debouncer.cpp \
    utilities/app_instances_shared_memory.cpp \
    utilities/async_routine.cpp \
    utilities/geometry_util.cpp \
    utilities/json_util.cpp \
    utilities/logging.cpp \
    utilities/message_box.cpp \
    utilities/periodic_checker.cpp \
    utilities/periodic_timer.cpp \
    utilities/save_debouncer.cpp \
    utilities/strings_util.cpp \
    widgets/board_view.cpp \
    widgets/board_view_toolbar.cpp \
    widgets/boards_list.cpp \
    widgets/card_properties_view.cpp \
    widgets/components/custom_graphics_text_item.cpp \
    widgets/components/custom_list_widget.cpp \
    widgets/components/custom_text_edit.cpp \
    widgets/components/edge_arrow.cpp \
    widgets/components/graphics_item_move_resize.cpp \
    widgets/components/graphics_scene.cpp \
    widgets/components/node_rect.cpp \
    widgets/components/simple_toolbar.cpp \
    widgets/dialogs/dialog_create_relationship.cpp \
    widgets/dialogs/dialog_set_labels.cpp \
    widgets/dialogs/dialog_user_card_labels.cpp \
    widgets/dialogs/dialog_user_relationship_types.cpp \
    widgets/main_window.cpp \
    widgets/right_sidebar.cpp \
    widgets/right_sidebar_toolbar.cpp

HEADERS += \
    app_data.h \
    app_event.h \
    db_access/abstract_boards_data_access.h \
    db_access/abstract_cards_data_access.h \
    db_access/boards_data_access.h \
    db_access/cards_data_access.h \
    db_access/queued_db_access.h \
    file_access/app_local_data_dir.h \
    file_access/local_settings_file.h \
    file_access/unsaved_update_records_file.h \
    global_constants.h \
    models/board.h \
    models/boards_list_properties.h \
    models/card.h \
    models/edge_arrow_data.h \
    models/node_labels.h \
    models/node_rect_data.h \
    models/relationship.h \
    neo4j_http_api_client.h \
    persisted_data_access.h \
    services.h \
    utilities/action_debouncer.h \
    utilities/app_instances_shared_memory.h \
    utilities/async_routine.h \
    utilities/functor.h \
    utilities/geometry_util.h \
    utilities/hash.h \
    utilities/json_util.h \
    utilities/lists_vectors_util.h \
    utilities/logging.h \
    utilities/maps_util.h \
    utilities/margins_util.h \
    utilities/message_box.h \
    utilities/numbers_util.h \
    utilities/periodic_checker.h \
    utilities/periodic_timer.h \
    utilities/save_debouncer.h \
    utilities/strings_util.h \
    widgets/board_view.h \
    widgets/board_view_toolbar.h \
    widgets/boards_list.h \
    widgets/card_properties_view.h \
    widgets/components/custom_graphics_text_item.h \
    widgets/components/custom_list_widget.h \
    widgets/components/custom_text_edit.h \
    widgets/components/edge_arrow.h \
    widgets/components/graphics_item_move_resize.h \
    widgets/components/graphics_scene.h \
    widgets/components/node_rect.h \
    widgets/components/simple_toolbar.h \
    widgets/dialogs/dialog_create_relationship.h \
    widgets/dialogs/dialog_set_labels.h \
    widgets/dialogs/dialog_user_card_labels.h \
    widgets/dialogs/dialog_user_relationship_types.h \
    widgets/main_window.h \
    widgets/right_sidebar.h \
    widgets/right_sidebar_toolbar.h

FORMS += \
    widgets/dialogs/dialog_create_relationship.ui \
    widgets/dialogs/dialog_set_labels.ui \
    widgets/dialogs/dialog_user_card_labels.ui \
    widgets/dialogs/dialog_user_relationship_types.ui \
    widgets/main_window.ui

RESOURCES += \
    resources.qrc

OTHER_FILES += \
    ../README.md


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


DEFINES += QT_MESSAGELOGCONTEXT
