#pragma once
#include "Core/D3D12Include.h"

namespace aZero
{
	namespace ECS
	{
		class PointLightComponent
		{
		private:
			DXM::Vector3 m_Position;
			DXM::Vector3 m_Color;
			float m_Range;

		public:
			PointLightComponent() = default;

			PointLightComponent(const DXM::Vector3& Position, const DXM::Vector3& Color, float Range)
			{
				m_Position = Position;
				m_Color = Color;
				m_Range = Range;
			}

			const DXM::Vector3& GetPosition() const { return m_Position; }
			const DXM::Vector3& GetColor() const { return m_Color; }
			const float& GetRange() const { return m_Range; }

			void SetPosition(const DXM::Vector3& Position) { m_Position = Position; }
			void SetColor(const DXM::Vector3& Color) { m_Color = Color; }
			void SetRange(float Range) { m_Range = Range; }
		};
	}
}