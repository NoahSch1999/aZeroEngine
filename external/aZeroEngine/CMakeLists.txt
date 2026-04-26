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

# Meshoptimizer
FetchContent_Declare(
    meshoptimizer
    GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
    GIT_TAG        "master"
)
FetchContent_MakeAvailable(meshoptimizer)
#

# aZero Audio
FetchContent_Declare(
    aZeroAudio
    GIT_REPOSITORY https://github.com/NoahSch1999/aZero-Audio.git
    GIT_TAG        "main"
)
FetchContent_MakeAvailable(aZeroAudio)
#

# YAML-CPP
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG        "master"
)
FetchContent_MakeAvailable(yaml-cpp)
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

add_library(aZeroEngine
    "src/misc/HelperFunctions.cpp"
    "src/misc/STBIHack.cpp"
    "src/renderer/Renderer.cpp"
    "src/assets/Mesh.cpp" 
    "src/assets/Material.cpp" 
    "src/assets/Texture.cpp"
    "src/graphics_api/Engine.cpp" 
    "src/scene/Scene.cpp" 
    "src/pipeline/shader/Shader.cpp" 
    "src/pipeline/shader/VertexShader.cpp" 
    "src/pipeline/shader/PixelShader.cpp" 
    "src/pipeline/shader/ComputeShader.cpp"
    "src/pipeline/shader/MeshShader.cpp" 
    "src/pipeline/pass/ComputeShaderPass.cpp" 
    "src/pipeline/pass/VertexShaderPass.cpp"
    "src/pipeline/pass/MeshShaderPass.cpp"
    "src/pipeline/pass/ShaderPassBase.cpp"  
    "src/pipeline/pass/MultiShaderPass.cpp" 
    "src/scene/SceneProxy.cpp" 
    "src/graphics_api/resource/ResourceBase.cpp"
    "src/graphics_api/resource/buffer/Buffer.cpp" 
    "src/graphics_api/resource/texture/Texture2D.cpp"
    "src/graphics_api/command_recording/CommandList.cpp"
    "src/graphics_api/command_recording/CommandQueue.cpp"
    "src/graphics_api/descriptor/Descriptor.cpp"
    "src/graphics_api/descriptor/DescriptorHeap.cpp"
    "src/graphics_api/SwapChain.cpp" "src/pipeline/shader/AmplificationShader.cpp")

target_sources(aZeroEngine PUBLIC ${ENGINE_HEADERS})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(aZeroEngine PUBLIC USE_DEBUG)
    target_compile_options(aZeroEngine PRIVATE -O0 -g)   # Disable optimization, enable debug info
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(aZeroEngine PRIVATE -O3)      # Full optimization
endif()

# PRE-PROCESSOR SETTINGS
target_compile_definitions(aZeroEngine PUBLIC 
    PROJECT_DIRECTORY="${PROJECT_DIRECTORY}"
    NOMINMAX
)

target_include_directories(aZeroEngine
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC ${CMAKE_BINARY_DIR}/_deps/aZero-sparse-set-src/src/include
    PUBLIC ${CMAKE_BINARY_DIR}/_deps/aZero-linear-allocator-src/include
)

# Linking
target_link_libraries(aZeroEngine 
    PUBLIC d3d12
    PUBLIC dxgi
    PUBLIC DirectX-Headers
    PUBLIC DirectX-Guids
    PUBLIC DirectXTK12
    PUBLIC DXCompiler
    PUBLIC assimp
    PUBLIC yaml-cpp
    PUBLIC aZeroAudio
    PUBLIC meshoptimizer
    #PUBLIC DXGIDebug.dll
)
#

target_precompile_headers(aZeroEngine PUBLIC
    <${CMAKE_BINARY_DIR}/_deps/aZero-linear-allocator-src/include/LinearAllocator.hpp>
)

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