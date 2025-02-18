#pragma once
#include "graphics_api/D3D12Include.hpp"

namespace aZero
{
	namespace ECS
	{
		class TransformComponent
		{
		private:
			DXM::Matrix m_Transform = DXM::Matrix::Identity;

		public:
			TransformComponent() = default;
			TransformComponent(const DXM::Matrix& Transform)
			{
				this->SetTransform(Transform);
			}

			void SetTransform(const DXM::Matrix& Transform) 
			{ 
				m_Transform = Transform; 
			}

			const DXM::Matrix& GetTransform() const 
			{ 
				return m_Transform; 
			}
		};
	}
}