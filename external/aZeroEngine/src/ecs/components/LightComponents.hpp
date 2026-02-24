#pragma once
namespace aZero
{
	namespace ECS
	{
		struct LightComponentBase
		{
			DXM::Vector3 Color;
			float Intensity;

			LightComponentBase() = default;
			LightComponentBase(const DXM::Vector3& color, float intensity)
				:Color(color), Intensity(intensity) {
			}
		};

		struct DirectionalLightComponent : public LightComponentBase
		{
			DXM::Vector3 Direction;
			DirectionalLightComponent() = default;
			DirectionalLightComponent(const DXM::Vector3& color, float intensity, const DXM::Vector3& direction)
				:LightComponentBase(color, intensity), Direction(direction) {
			}
		};

		struct PointLightComponent : public LightComponentBase
		{
			DXM::Vector3 Position;
			float FalloffFactor;

			PointLightComponent() = default;
			PointLightComponent(const DXM::Vector3& color, float intensity, const DXM::Vector3& position, float falloffFactor)
				:LightComponentBase(color, intensity), Position(position), FalloffFactor(falloffFactor) {
			}
		};

		struct SpotLightComponent : public LightComponentBase
		{
			DXM::Vector3 Position;
			DXM::Vector3 Direction;
			float Range;

			// Cos of angle that is valid between light vector to point and direction
			// 1 is smaller cone, 0 is 90 deg cone
			float CutoffAngle;

			SpotLightComponent() = default;
			SpotLightComponent(const DXM::Vector3& color, float intensity,
				const DXM::Vector3& direction, const DXM::Vector3& position, float range, float cutoffAngle)
				:LightComponentBase(color, intensity), Direction(direction), Position(position), Range(range), CutoffAngle(cutoffAngle) {
			}
		};
	}
}