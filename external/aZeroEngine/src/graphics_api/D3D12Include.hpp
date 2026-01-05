#pragma once
#include <Windows.h>
#include "directx/d3dx12.h"
#include <wrl.h>
#include <dxgi1_5.h>
#include <d3dcompiler.h>
#include <SimpleMath.h>
#include <stdexcept>
#include "misc/EngineDebugMacros.hpp"
#include "graphics_api/DXCompilerInclude.hpp"

#if USE_DEBUG
#include <dxgidebug.h>
#endif // DEBUG

namespace DXM = DirectX::SimpleMath;

// Dx12 api interfaces that might need an upgrade to a newer version at some point. So they are aliased to avoid having to replace all in the entire project whenever we wanna change the version.
using ID3D12DeviceX = ID3D12Device9;
using ID3D12FenceX = ID3D12Fence1;
using ID3D12GraphicsCommandListX = ID3D12GraphicsCommandList7;
using IDxcCompilerX = IDxcCompiler3;