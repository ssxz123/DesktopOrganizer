add_library(organizer_core STATIC
    classifier.cpp
    rules.cpp
    organizer.cpp
    undo_log.cpp
    config.cpp
    desktop_watch.cpp
)

target_include_directories(organizer_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(organizer_core PUBLIC Qt6::Core)
