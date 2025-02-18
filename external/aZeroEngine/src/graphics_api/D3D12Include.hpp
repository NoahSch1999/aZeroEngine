#pragma once
#include <Windows.h>
#include "directx/d3dx12.h"
#include <wrl.h>
#include <dxgi1_5.h>
#include <d3dcompiler.h>
#include <SimpleMath.h>
#include <stdexcept>
#include "misc/EngineDebugMacros.hpp"

#if USE_DEBUG
#include <dxgidebug.h>
#endif // DEBUG

namespace DXM = DirectX::SimpleMath;