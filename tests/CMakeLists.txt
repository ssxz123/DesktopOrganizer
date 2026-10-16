set(TEST_TARGETS
    test_classifier
    test_rules
    test_organizer
    test_undo_log
)

# MinGW 运行时 DLL 所在目录
set(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}")
cmake_path(GET MINGW_BIN_DIR PARENT_PATH MINGW_BIN_DIR)

# 测试可执行文件需要的 DLL（Windows 加载器优先搜索 exe 所在目录，最可靠）
set(TEST_DLLS
    "${Qt6Core_DIR}/../../../bin/Qt6Core.dll"
    "${Qt6Core_DIR}/../../../bin/Qt6Test.dll"
)

foreach(target ${TEST_TARGETS})
    add_executable(${target} ${target}.cpp)
    target_link_libraries(${target} PRIVATE organizer_core Qt6::Test)
    add_test(NAME ${target} COMMAND ${target})

    if(WIN32)
        foreach(dll ${TEST_DLLS})
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                        "${dll}" "$<TARGET_FILE_DIR:${target}>"
                VERBATIM)
        endforeach()
        foreach(runtime_dll libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                        "${MINGW_BIN_DIR}/${runtime_dll}" "$<TARGET_FILE_DIR:${target}>"
                VERBATIM)
        endforeach()
    endif()
endforeach()
