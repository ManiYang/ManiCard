QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    app_data.cpp \
    app_data_readonly.cpp \
    application.cpp \
    db_access/abstract_boards_data_access.cpp \
    db_access/abstract_cards_data_access.cpp \
    db_access/boards_data_access.cpp \
    db_access/cards_data_access.cpp \
    db_access/debounced_db_access.cpp \
    db_access/queued_db_access.cpp \
    file_access/local_settings_file.cpp \
    file_access/unsaved_update_records_file.cpp \
    main.cpp \
    models/board.cpp \
    models/card.cpp \
    models/custom_data_query.cpp \
    models/data_view_box_data.cpp \
    models/group_box_data.cpp \
    models/group_box_tree.cpp \
    models/node_rect_data.cpp \
    models/relationship.cpp \
    models/relationships_bundle.cpp \
    models/setting_box_data.cpp \
    models/settings/abstract_setting.cpp \
    models/settings/card_label_color_mapping.cpp \
    models/settings/card_properties_to_show.cpp \
    models/settings/settings.cpp \
    models/workspace.cpp \
    models/workspaces_list_properties.cpp \
    neo4j_http_api_client.cpp \
    persisted_data_access.cpp \
    services.cpp \
    utilities/action_debouncer.cpp \
    utilities/app_instances_shared_memory.cpp \
    utilities/async_routine.cpp \
#    utilities/directed_graph.cpp \
    utilities/fonts_util.cpp \
    utilities/geometry_util.cpp \
    utilities/json_util.cpp \
    utilities/logging.cpp \
    utilities/map_update.cpp \
    utilities/message_box.cpp \
    utilities/periodic_checker.cpp \
    utilities/periodic_timer.cpp \
    utilities/screens_utils.cpp \
    utilities/strings_util.cpp \
    widgets/app_style_sheet.cpp \
    widgets/board_view.cpp \
    widgets/board_view_toolbar.cpp \
    widgets/card_properties_view.cpp \
    widgets/components/board_box_item.cpp \
    widgets/components/custom_graphics_text_item.cpp \
    widgets/components/custom_list_widget.cpp \
    widgets/components/custom_tab_bar.cpp \
    widgets/components/custom_text_edit.cpp \
    widgets/components/data_view_box.cpp \
    widgets/components/drag_point_events_handler.cpp \
    widgets/components/edge_arrow.cpp \
    widgets/components/graphics_item_move_resize.cpp \
    widgets/components/graphics_scene.cpp \
    widgets/components/group_box.cpp \
    widgets/components/node_rect.cpp \
    widgets/components/property_value_editor.cpp \
    widgets/components/setting_box.cpp \
    widgets/components/simple_toolbar.cpp \
    widgets/dialogs/dialog_create_relationship.cpp \
    widgets/dialogs/dialog_options.cpp \
    widgets/dialogs/dialog_set_labels.cpp \
    widgets/dialogs/dialog_user_card_labels.cpp \
    widgets/dialogs/dialog_user_relationship_types.cpp \
    widgets/dialogs/dialog_workspace_card_colors.cpp \
    widgets/icons.cpp \
    widgets/main_window.cpp \
    widgets/right_sidebar.cpp \
    widgets/right_sidebar_toolbar.cpp \
    widgets/workspace_frame.cpp \
    widgets/workspaces_list.cpp

HEADERS += \
    app_data.h \
    app_data_readonly.h \
    app_event_source.h \
    application.h \
    db_access/abstract_boards_data_access.h \
    db_access/abstract_cards_data_access.h \
    db_access/boards_data_access.h \
    db_access/cards_data_access.h \
    db_access/debounced_db_access.h \
    db_access/queued_db_access.h \
    file_access/app_local_data_dir.h \
    file_access/local_settings_file.h \
    file_access/unsaved_update_records_file.h \
    global_constants.h \
    models/board.h \
    models/card.h \
    models/custom_data_query.h \
    models/data_view_box_data.h \
    models/edge_arrow_data.h \
    models/group_box_data.h \
    models/group_box_tree.h \
    models/node_labels.h \
    models/node_rect_data.h \
    models/relationship.h \
    models/relationships_bundle.h \
    models/setting_box_data.h \
    models/settings/abstract_setting.h \
    models/settings/card_label_color_mapping.h \
    models/settings/card_properties_to_show.h \
    models/settings/settings.h \
    models/workspace.h \
    models/workspaces_list_properties.h \
    neo4j_http_api_client.h \
    persisted_data_access.h \
    services.h \
    utilities/action_debouncer.h \
    utilities/app_instances_shared_memory.h \
    utilities/async_routine.h \
    utilities/binary_search.h \
#    utilities/directed_graph.h \
    utilities/colors_util.h \
    utilities/fonts_util.h \
    utilities/functor.h \
    utilities/geometry_util.h \
    utilities/hash.h \
    utilities/json_util.h \
    utilities/lists_vectors_util.h \
    utilities/logging.h \
    utilities/map_update.h \
    utilities/maps_util.h \
    utilities/margins_util.h \
    utilities/message_box.h \
    utilities/naming_rules.h \
    utilities/numbers_util.h \
    utilities/periodic_checker.h \
    utilities/periodic_timer.h \
    utilities/screens_utils.h \
    utilities/sets_util.h \
    utilities/strings_util.h \
    utilities/style_sheet_util.h \
    utilities/variables_update_propagator.h \
    widgets/app_style_sheet.h \
    widgets/board_view.h \
    widgets/board_view_toolbar.h \
    widgets/card_properties_view.h \
    widgets/common_types.h \
    widgets/components/board_box_item.h \
    widgets/components/custom_graphics_text_item.h \
    widgets/components/custom_list_widget.h \
    widgets/components/custom_tab_bar.h \
    widgets/components/custom_text_edit.h \
    widgets/components/data_view_box.h \
    widgets/components/drag_point_events_handler.h \
    widgets/components/edge_arrow.h \
    widgets/components/graphics_item_move_resize.h \
    widgets/components/graphics_scene.h \
    widgets/components/group_box.h \
    widgets/components/node_rect.h \
    widgets/components/property_value_editor.h \
    widgets/components/setting_box.h \
    widgets/components/simple_toolbar.h \
    widgets/dialogs/dialog_create_relationship.h \
    widgets/dialogs/dialog_options.h \
    widgets/dialogs/dialog_set_labels.h \
    widgets/dialogs/dialog_user_card_labels.h \
    widgets/dialogs/dialog_user_relationship_types.h \
    widgets/dialogs/dialog_workspace_card_colors.h \
    widgets/icons.h \
    widgets/main_window.h \
    widgets/right_sidebar.h \
    widgets/right_sidebar_toolbar.h \
    widgets/widgets_constants.h \
    widgets/workspace_frame.h \
    widgets/workspaces_list.h

FORMS += \
    widgets/dialogs/dialog_create_relationship.ui \
    widgets/dialogs/dialog_options.ui \
    widgets/dialogs/dialog_set_labels.ui \
    widgets/dialogs/dialog_user_card_labels.ui \
    widgets/dialogs/dialog_user_relationship_types.ui \
    widgets/dialogs/dialog_workspace_card_colors.ui \
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

#include(boost_dependency.pri)
