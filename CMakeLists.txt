cmake_minimum_required (VERSION 3.20)

project (aZeroEditor VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_definitions(aZeroEditor NOMINMAX)

add_subdirectory(${PROJECT_SOURCE_DIR}/idk/aZeroEngine)

add_executable(aZeroEditor WIN32 "main.cpp" "aZeroEditor.hpp" )

target_include_directories(aZeroEditor
    PUBLIC ${PROJECT_SOURCE_DIR}/idk/aZeroEngine/include
)

target_link_directories(aZeroEditor
    PUBLIC ${CMAKE_BINARY_DIR}idk/aZeroEngine
)

target_link_libraries(aZeroEditor 
    PUBLIC aZeroEngine
)