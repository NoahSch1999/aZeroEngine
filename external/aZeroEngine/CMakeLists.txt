cmake_minimum_required (VERSION 3.20)

project (aZeroEngine VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)

add_library(aZeroEngine
    "src/graphics_api/resource_type/GPUResource.cpp"
    "src/graphics_api/Descriptor.cpp"
    "src/misc/HelperFunctions.cpp"
    "src/misc/STBIHack.cpp"
    "src/renderer/render_asset/RenderAssetData.cpp"
    "src/renderer/render_graph/Shader.cpp"
    "src/renderer/Renderer.cpp"
    "src/window/RenderWindow.cpp"
)

# PRE-PROCESSOR SETTINGS
target_compile_definitions(aZeroEngine PUBLIC USE_DEBUG)

# Assimp
FetchContent_Declare(Assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
     GIT_TAG "v5.4.3"
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
FetchContent_MakeAvailable(DirectX-Headers)
#

# DirectX Toolkit
FetchContent_Declare(
  DirectXTK12
  GIT_REPOSITORY https://github.com/microsoft/DirectXTK12.git
  GIT_TAG        "main"
)
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

target_include_directories(aZeroEngine 
    # For aZeroEngine local includes
    #PUBLIC ${PROJECT_SOURCE_DIR}/src
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src

	PUBLIC ${CMAKE_BINARY_DIR}/_deps/dxcompiler-src/inc
    PUBLIC ${CMAKE_BINARY_DIR}/_deps/directx-headers-src/include
    PUBLIC ${CMAKE_BINARY_DIR}/_deps/directxtk12-src/Inc
    PUBLIC ${CMAKE_BINARY_DIR}/_deps/assimp-build/include
    PUBLIC ${CMAKE_BINARY_DIR}/_deps/assimp-src/include
)

target_link_directories(aZeroEngine
    PUBLIC ${CMAKE_BINARY_DIR}/_deps/assimp-build/lib
    PUBLIC ${CMAKE_BINARY_DIR}/_deps/assimp-build/contrib/zlib
)

# Linking
target_link_libraries(aZeroEngine 
    PUBLIC d3d12
    PUBLIC dxgi
    PUBLIC DirectX-Headers
    PUBLIC DirectX-Guids
    PUBLIC DirectXTK12
    PUBLIC DXCompiler
    PUBLIC assimp-vc143-mt
    PUBLIC zlibstaticd
    #PUBLIC DXGIDebug.dll
)
#

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