#pragma once
#include "graphics_api/D3D12Include.hpp"

namespace aZero
{
	struct LightBaseData
	{
		DXM::Vector3 Color;
		float Intensity;
	};

	struct DirectionalLightData : public LightBaseData
	{
		DXM::Vector3 Direction;
	};

	struct PointLightData : public LightBaseData
	{
		DXM::Vector3 Position;
		float FalloffFactor;
	};

	struct SpotLightData : public LightBaseData
	{
		DXM::Vector3 Position;
		DXM::Vector3 Direction;
		float Range;

		// Cos of angle that is valid between light vector to point and direction
		// 1 is smaller cone, 0 is 90 deg cone
		float CutoffAngle;
	};
}