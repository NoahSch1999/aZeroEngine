#pragma once
#include "Core/D3D12Include.h"

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