cmake_minimum_required (VERSION 3.23)

project (aZeroEditor VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(aZeroEditor NOMINMAX)

add_subdirectory(${PROJECT_SOURCE_DIR}/external/aZeroEngine)

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

target_include_directories(aZeroEditor
    PRIVATE ${PROJECT_SOURCE_DIR}/external/aZeroEngine/include
    PRIVATE ${CMAKE_BINARY_DIR}/_deps/imgui-src
    PRIVATE ${CMAKE_BINARY_DIR}/_deps/imgui-src
    PRIVATE ${CMAKE_BINARY_DIR}/_deps/imgui-src
    PRIVATE ${CMAKE_BINARY_DIR}/_deps/SDLCppWrapper-src
)

target_link_directories(aZeroEditor
    PRIVATE ${CMAKE_BINARY_DIR}external/aZeroEngine
)

target_link_libraries(aZeroEditor 
    PRIVATE 
        aZeroEngine
        SDLCppWrapper
)

# ImGui
FetchContent_Declare(
  ImGui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        "docking"
)
FetchContent_MakeAvailable(ImGui)
#

# SDLCppWrapper
FetchContent_Declare(
  SDLCppWrapper
  GIT_REPOSITORY https://github.com/NoahSch1999/aZero-SDL-Cpp-Wrapper.git
  GIT_TAG        "main"
)
FetchContent_MakeAvailable(SDLCppWrapper)
#