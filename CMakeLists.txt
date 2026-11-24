cmake_minimum_required(VERSION 3.20)
project(gw2_nexus_timer VERSION 0.1.1 LANGUAGES CXX)
set(GW2_TIMER_GITHUB_REPOSITORY "tjhhhhhwwh/gw2-nexus-timer" CACHE STRING "Optional owner/repository for Nexus GitHub Releases updates")
if(GW2_TIMER_GITHUB_REPOSITORY AND NOT GW2_TIMER_GITHUB_REPOSITORY MATCHES "^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$")
    message(FATAL_ERROR "GW2_TIMER_GITHUB_REPOSITORY must be owner/repository")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(BOSS_IMAGE_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/boss_images.generated.h")
file(GLOB BOSS_IMAGE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/bosses/*.jpg")
add_custom_command(OUTPUT "${BOSS_IMAGE_HEADER}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_boss_images.py"
            --output "${BOSS_IMAGE_HEADER}"
    DEPENDS tools/embed_boss_images.py data/bosses.json ${BOSS_IMAGE_FILES}
    VERBATIM)
add_custom_target(embed_boss_images DEPENDS "${BOSS_IMAGE_HEADER}")

add_library(timer_core STATIC src/schedule.cpp)
target_include_directories(timer_core PUBLIC src)
if(MSVC)
    target_compile_options(timer_core PUBLIC /utf-8 /W4)
else()
    target_compile_options(timer_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(schedule_tests tests/schedule_tests.cpp)
    target_link_libraries(schedule_tests PRIVATE timer_core)
    add_test(NAME schedule_tests COMMAND schedule_tests)
    add_executable(boss_images_tests tests/boss_images_tests.cpp)
    add_dependencies(boss_images_tests embed_boss_images)
    target_include_directories(boss_images_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
    target_link_libraries(boss_images_tests PRIVATE timer_core)
    add_test(NAME boss_images_tests COMMAND boss_images_tests "${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME boss_assets_tests COMMAND "${Python3_EXECUTABLE}" -m unittest discover
             -s "${CMAKE_CURRENT_SOURCE_DIR}/tests" -p test_boss_assets.py)
endif()

if(WIN32)
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "Nexus requires a Windows x64 build. Use -A x64.")
    endif()
    add_library(nexus_imgui STATIC
        third_party/imgui/imgui.cpp
        third_party/imgui/imgui_draw.cpp
        third_party/imgui/imgui_tables.cpp
        third_party/imgui/imgui_widgets.cpp)
    target_include_directories(nexus_imgui PUBLIC third_party/imgui)
    add_library(gw2_nexus_timer SHARED src/addon.cpp src/clipboard_win.cpp)
    add_dependencies(gw2_nexus_timer embed_boss_images)
    target_include_directories(gw2_nexus_timer PRIVATE third_party/nexus "${CMAKE_CURRENT_BINARY_DIR}/generated")
    target_link_libraries(gw2_nexus_timer PRIVATE timer_core nexus_imgui user32)
    target_compile_definitions(gw2_nexus_timer PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    if(GW2_TIMER_GITHUB_REPOSITORY)
        target_compile_definitions(gw2_nexus_timer PRIVATE
            GW2_TIMER_GITHUB_URL="https://github.com/${GW2_TIMER_GITHUB_REPOSITORY}")
    endif()
    set_target_properties(gw2_nexus_timer PROPERTIES PREFIX "" OUTPUT_NAME "gw2_nexus_timer")
    install(TARGETS gw2_nexus_timer RUNTIME DESTINATION .)
else()
    message(STATUS "Building portable schedule tests only; the Nexus DLL requires Windows x64.")
endif()
