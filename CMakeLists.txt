cmake_minimum_required (VERSION 3.23)

project (aZeroEditor VERSION 1.0)

option(RUN_TESTS "Run tests" OFF)
set(PROJECT_DIRECTORY "${CMAKE_SOURCE_DIR}/content/" CACHE STRING "Project directory path")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(${PROJECT_SOURCE_DIR}/external/aZeroEngine)

# ImGui
FetchContent_Declare(
  ImGui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        "docking"
)
FetchContent_MakeAvailable(ImGui)
#

# aZero Window
FetchContent_Declare(
  aZeroWindow
  GIT_REPOSITORY https://github.com/NoahSch1999/aZero-Window.git
  GIT_TAG        "main"
)
FetchContent_MakeAvailable(aZeroWindow)
#

# aZero Input
FetchContent_Declare(
  aZeroInput
  GIT_REPOSITORY https://github.com/NoahSch1999/aZero-Input.git
  GIT_TAG        "main"
)
FetchContent_MakeAvailable(aZeroInput)
#

add_executable(aZeroEditor
    "main.cpp" 
    "aZeroEditor.hpp"
    "${CMAKE_BINARY_DIR}/_deps/imgui-src/imgui.cpp"
    "${CMAKE_BINARY_DIR}/_deps/imgui-src/backends/imgui_impl_dx12.cpp"
    "${CMAKE_BINARY_DIR}/_deps/imgui-src/backends/imgui_impl_win32.cpp"
    "${CMAKE_BINARY_DIR}/_deps/imgui-src/imgui_widgets.cpp"
    "${CMAKE_BINARY_DIR}/_deps/imgui-src/imgui_tables.cpp"
    "${CMAKE_BINARY_DIR}/_deps/imgui-src/imgui_draw.cpp"
    "${CMAKE_BINARY_DIR}/_deps/imgui-src/imgui_demo.cpp"
)

if (RUN_TESTS)
    target_compile_definitions(aZeroEditor PUBLIC RUN_TESTS)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(aZeroEditor PUBLIC USE_DEBUG)
    target_compile_options(aZeroEditor PRIVATE -O0 -g)   # Disable optimization, enable debug info
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(aZeroEditor PRIVATE -O3)      # Full optimization
endif()

target_compile_definitions(aZeroEditor PUBLIC TEST_PATH="${CMAKE_SOURCE_DIR}/tests/")

target_include_directories(aZeroEditor
    PRIVATE ${CMAKE_BINARY_DIR}/_deps/imgui-src
)

target_link_libraries(aZeroEditor 
    PRIVATE 
        aZeroEngine
        aZeroWindow
        aZeroInput
)
