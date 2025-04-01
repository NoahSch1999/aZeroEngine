cmake_minimum_required (VERSION 3.20)

project (aZeroEditor VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_definitions(aZeroEditor NOMINMAX)

add_subdirectory(${PROJECT_SOURCE_DIR}/external/aZeroEngine)

add_executable(aZeroEditor WIN32 
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
    PUBLIC ${PROJECT_SOURCE_DIR}/external/aZeroEngine/include
    PUBLIC ${CMAKE_BINARY_DIR}/_deps/imgui-src
)

target_link_directories(aZeroEditor
    PUBLIC ${CMAKE_BINARY_DIR}external/aZeroEngine
)

target_link_libraries(aZeroEditor 
    PUBLIC aZeroEngine
)

# ImGui
FetchContent_Declare(
  ImGui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        "docking"
)
FetchContent_MakeAvailable(ImGui)
#