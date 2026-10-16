set(APP_SOURCES
    main.cpp
    main_window.cpp
    main_window.h
    main_window.ui
    rules_dialog.cpp
    rules_dialog.h
    rules_dialog.ui
    settings_dialog.cpp
    settings_dialog.h
    settings_dialog.ui
    resources.qrc
)

add_executable(desktop_organizer ${APP_SOURCES})

target_link_libraries(desktop_organizer PRIVATE organizer_core Qt6::Widgets)

# 构建后自动用 windeployqt 拷贝 Qt DLL，双击 exe 也能运行
find_program(WINDEPLOYQT_EXE windeployqt
    HINTS "${Qt6Core_DIR}/../../../bin"
    REQUIRED)

add_custom_command(TARGET desktop_organizer POST_BUILD
    COMMAND "${WINDEPLOYQT_EXE}" --no-translations "$<TARGET_FILE:desktop_organizer>"
    COMMENT "Running windeployqt to deploy Qt runtime..."
    VERBATIM)
