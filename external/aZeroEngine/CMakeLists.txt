cmake_minimum_required (VERSION 3.23)

project (aZeroEngine VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(PROJECT_DIRECTORY "${CMAKE_SOURCE_DIR}" CACHE STRING "Project directory path")

include(FetchContent)

# Assimp
FetchContent_Declare(Assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG "v6.0.4"
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(DASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(Assimp)
#

# DirectX-Headers
FetchContent_Declare(
  DirectX-Headers
  GIT_REPOSITORY https://github.com/microsoft/DirectX-Headers.git
  GIT_TAG        "v1.614.1"
)
set(DXHEADERS_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(DXHEADERS_INSTALL OFF CACHE BOOL "" FORCE)
set(DXHEADERS_BUILD_GOOGLE_TEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(DirectX-Headers)
#

# DirectX Toolkit
FetchContent_Declare(
  DirectXTK12
  GIT_REPOSITORY https://github.com/microsoft/DirectXTK12.git
  GIT_TAG        "main"
)
set(BUILD_XAUDIO_WIN10 OFF CACHE BOOL "" FORCE)
set(BUILD_DXIL_SHADERS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(DirectXTK12)
#

# DXCompiler
FetchContent_Declare(
    DXCompiler
    URL https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2407/dxc_2024_07_31.zip
    URL_HASH SHA256=e2627f004f0f9424d8c71ea1314d04f38c5a5096884ae9217f1f18bd320267b5
    DOWNLOAD_NAME DXCompiler.zip
)
FetchContent_MakeAvailable(DXCompiler)
#

# Agility SDK
FetchContent_Declare(
    AgilitySDK
    URL https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.614.0
    URL_HASH SHA256=f8ee4da4851d0bba97d232e830565804743d95a4ae37a66a5b5757ac29668d77
    DOWNLOAD_NAME AgilitySDK.zip
)
FetchContent_MakeAvailable(AgilitySDK)
#

# PIX
FetchContent_Declare(
    winpix
    URL https://www.nuget.org/api/v2/package/WinPixEventRuntime/1.0.240308001
)

FetchContent_MakeAvailable(winpix)
#

# Meshoptimizer
FetchContent_Declare(
    meshoptimizer
    GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
    GIT_TAG        "master"
)
FetchContent_MakeAvailable(meshoptimizer)
#

# Jolt Physics
FetchContent_Declare(
    joltPhysics
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG        "master"
    SOURCE_SUBDIR "Build"
)
#set(USE_ASSERTS OFF)
#set(JPH_BUILD_SHARED_LIBS OFF)
##set(JPH_CPP_EXCEPTIONS_ENABLED OFF)
#set(JPH_FLOATING_POINT_EXCEPTIONS_ENABLED ON)
#set(JPH_CROSS_PLATFORM_DETERMINISTIC OFF)
#set(GENERATE_DEBUG_SYMBOLS ON)
#set(JPH_DOUBLE_PRECISION OFF)
#set(JPH_OBJECT_LAYER_BITS 16)
#set(JPH_USE_DX12 ON)
#option(CPP_RTTI_ENABLED "Enable C++ RTTI" OFF)
FetchContent_MakeAvailable(joltPhysics)
#

# Flecs
FetchContent_Declare(
    flecs
    GIT_REPOSITORY https://github.com/SanderMertens/flecs.git
    GIT_TAG master
)
 
FetchContent_MakeAvailable(flecs)
#

# aZero Audio
FetchContent_Declare(
    aZeroAudio
    GIT_REPOSITORY https://github.com/NoahSch1999/aZero-Audio.git
    GIT_TAG        "main"
)
FetchContent_MakeAvailable(aZeroAudio)
#

# SIMDJSON
FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson.git
    GIT_TAG        "master"
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(simdjson)
#

# aZero Sparse Set
FetchContent_Declare(
    aZero-Sparse-Set
    GIT_REPOSITORY https://github.com/NoahSch1999/aZero-Sparse-Set.git
    GIT_TAG        "master"
)
FetchContent_MakeAvailable(aZero-Sparse-Set)
#

# aZero Linear Allocator
FetchContent_Declare(
   aZero-Linear-Allocator
   GIT_REPOSITORY https://github.com/NoahSch1999/aZero-LinearAllocator.git
   GIT_TAG        "master"
)
FetchContent_MakeAvailable(aZero-Linear-Allocator)

file(GLOB ENGINE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.hpp"
)

add_library(${PROJECT_NAME}
    "src/misc/HelperFunctions.cpp"
    "src/misc/STBIHack.cpp"
    "src/renderer/Renderer.cpp"
    "src/assets/Mesh.cpp" 
    "src/assets/Material.cpp" 
    "src/assets/Texture.cpp"
    "src/engine/Engine.cpp" 
    "src/scene/Scene.cpp"
    "src/render_api/resource/ResourceBase.cpp"
    "src/render_api/resource/buffer/Buffer.cpp" 
    "src/render_api/resource/texture/Texture2D.cpp"
    "src/render_api/command_recording/CommandList.cpp"
    "src/render_api/command_recording/CommandQueue.cpp"
    "src/render_api/descriptor/Descriptor.cpp"
    "src/render_api/descriptor/DescriptorHeap.cpp"
    "src/render_api/SwapChain.cpp" 
    "src/physics/PhysicsEngine.cpp" 
    "src/physics/PhysicsWorld.cpp" 
    "src/ecs/ColliderComponent.cpp" 
    "src/renderer/WireframeRenderer.cpp" 
    "src/pipeline/Shader.cpp" 
    "src/pipeline/RenderPass.cpp"  "src/assets/MeshPrimtivies.cpp")

target_sources(${PROJECT_NAME} PUBLIC ${ENGINE_HEADERS})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PUBLIC USE_DEBUG USE_PIX)
    target_compile_options(${PROJECT_NAME} PRIVATE -O0 -g)   # Disable optimization, enable debug info
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${PROJECT_NAME} PRIVATE -O2)      # Full optimization
endif()

# PRE-PROCESSOR SETTINGS
target_compile_definitions(${PROJECT_NAME} PUBLIC 
    PROJECT_DIRECTORY="${PROJECT_DIRECTORY}"
    NOMINMAX
)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC ${CMAKE_BINARY_DIR}/_deps/aZero-sparse-set-src/src/include
    PUBLIC ${CMAKE_BINARY_DIR}/_deps/aZero-linear-allocator-src/include
    PUBLIC ${joltPhysics_SOURCE_DIR}/..
    PUBLIC ${winpix_SOURCE_DIR}/Include
)

target_link_directories(${PROJECT_NAME}
    PUBLIC ${winpix_SOURCE_DIR}/bin/x64
)

# Linking
target_link_libraries(${PROJECT_NAME} 
    PUBLIC d3d12
    PUBLIC dxgi
    PUBLIC DirectX-Headers
    PUBLIC DirectX-Guids
    PUBLIC DirectXTK12
    PUBLIC DXCompiler
    PUBLIC assimp
    PUBLIC simdjson
    PUBLIC aZeroAudio
    PUBLIC meshoptimizer
    PUBLIC Jolt
    PUBLIC WinPixEventRuntime
    PUBLIC flecs::flecs_static
    #PUBLIC DXGIDebug.dll
)
#

#target_precompile_headers(aZeroEngine PUBLIC
#    <${CMAKE_BINARY_DIR}/_deps/aZero-linear-allocator-src/include/LinearAllocator.hpp>
#)

#add_custom_command(TARGET aZeroEngine POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different
#        "${winpix_SOURCE_DIR}/bin/x64/WinPixEventRuntime.dll"
#        $<TARGET_FILE_DIR:aZeroEngine>
#)

file(COPY ${winpix_SOURCE_DIR}/bin/x64/WinPixEventRuntime.dll
     DESTINATION ${CMAKE_BINARY_DIR})

# DXCompiler
file(COPY ${CMAKE_BINARY_DIR}/_deps/dxcompiler-src/bin/x64/dxcompiler.dll
     DESTINATION ${CMAKE_BINARY_DIR})

file(COPY ${CMAKE_BINARY_DIR}/_deps/dxcompiler-src/bin/x64/dxil.dll
     DESTINATION ${CMAKE_BINARY_DIR})

# Agility SDK
file(COPY ${CMAKE_BINARY_DIR}/_deps/agilitysdk-src/build/native/bin/x64/D3D12Core.dll
     DESTINATION ${CMAKE_BINARY_DIR}/D3D12)

file(COPY ${CMAKE_BINARY_DIR}/_deps/agilitysdk-src/build/native/bin/x64/D3D12Core.pdb
     DESTINATION ${CMAKE_BINARY_DIR}/D3D12)

file(COPY ${CMAKE_BINARY_DIR}/_deps/agilitysdk-src/build/native/bin/x64/d3d12SDKLayers.dll
     DESTINATION ${CMAKE_BINARY_DIR}/D3D12)

file(COPY ${CMAKE_BINARY_DIR}/_deps/agilitysdk-src/build/native/bin/x64/d3d12SDKLayers.pdb
     DESTINATION ${CMAKE_BINARY_DIR}/D3D12)
#