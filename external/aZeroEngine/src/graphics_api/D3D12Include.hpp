#pragma once
#include <Windows.h>
#include "directx/d3dx12.h"
#include <wrl.h>
#include <dxgi.h>
#include <dxgi1_3.h>
#include <dxgi1_5.h>
#include <d3dcompiler.h>
#include <SimpleMath.h>
#include <stdexcept>
#include "misc/EngineDebugMacros.hpp"

#ifdef _DEBUG
#include <dxgidebug.h>
#endif // _DEBUG

namespace DXM = DirectX::SimpleMath;