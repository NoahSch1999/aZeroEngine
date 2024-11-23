#pragma once
#include "Core/D3D12Core.h"

namespace aZero
{
	namespace ECS
	{
		class DirectionalLightComponent
		{
		private:
			DXM::Vector3 m_Direction;
			DXM::Vector3 m_Color;

		public:
			DirectionalLightComponent() = default;

			DirectionalLightComponent(const DXM::Vector3& Direction, const DXM::Vector3& Color)
			{
				m_Direction = Direction;
				m_Color = Color;
			}

			const DXM::Vector3& GetDirection() const { return m_Direction; }
			const DXM::Vector3& GetColor() const { return m_Color; }

			void SetDirection(const DXM::Vector3& Direction) { m_Direction = Direction; }
			void SetColor(const DXM::Vector3& Color) { m_Color = Color; }
		};
	}
}