#pragma once
#include "Core/D3D12Include.h"

namespace aZero
{
	namespace ECS
	{
		class SpotLightComponent
		{
		private:
			DXM::Vector3 m_Position;
			DXM::Vector3 m_Color;
			float m_Range;
			float m_Radius;

		public:
			SpotLightComponent() = default;

			SpotLightComponent(const DXM::Vector3& Position, const DXM::Vector3& Color, float Range, float Radius)
			{
				m_Position = Position;
				m_Color = Color;
				m_Range = Range;
				m_Radius = Radius;
			}

			const DXM::Vector3& GetPosition() const { return m_Position; }
			const DXM::Vector3& GetColor() const { return m_Color; }
			const float& GetRange() const { return m_Range; }
			const float& GetRadius() const { return m_Radius; }

			void SetPosition(const DXM::Vector3& Position) { m_Position = Position; }
			void SetColor(const DXM::Vector3& Color) { m_Color = Color; }
			void SetRange(float Range) { m_Range = Range; }
			void SetRadius(float Radius) { m_Radius = Radius; }
		};
	}
}