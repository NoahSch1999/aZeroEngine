#pragma once
#include "graphics_api/D3D12Include.hpp"

namespace aZero
{
	namespace ECS
	{
		class CameraComponent
		{
		private:
			static constexpr DXM::Vector3 ForwardVector = DXM::Vector3(0, 0, 1);
			static constexpr DXM::Vector3 UpVector = DXM::Vector3(0, 1, 0);

		public:
			DXM::Vector3 m_Position = DXM::Vector3::Zero;
			DXM::Vector3 m_ForwardVector = ForwardVector;
			DXM::Vector3 m_UpVector = UpVector;

			DXM::Vector2 m_TopLeft;
			DXM::Vector2 m_Dimensions;
			float m_NearPlane;
			float m_FarPlane;
			float m_Fov;

			CameraComponent() = default;

			DXM::Matrix GetViewMatrix() const
			{
				return DXM::Matrix::CreateLookAt(m_Position, m_Position + m_ForwardVector, m_UpVector);
			}

			DXM::Matrix GetProjectionMatrix() const
			{
				return DXM::Matrix::CreatePerspectiveFieldOfView(m_Fov, m_Dimensions.x / m_Dimensions.y, m_NearPlane, m_FarPlane);
			}

			DXM::Matrix GetViewProjectionMatrix() const
			{
				return this->GetViewMatrix() * this->GetProjectionMatrix();
			}

			D3D12_VIEWPORT GetViewport() const
			{
				D3D12_VIEWPORT Viewport;
				Viewport.TopLeftX = m_TopLeft.x;
				Viewport.TopLeftY = m_TopLeft.y;
				Viewport.Width = m_Dimensions.x;
				Viewport.Height = m_Dimensions.y;
				Viewport.MinDepth = 0;
				Viewport.MaxDepth = 1;
				return Viewport;
			}

			D3D12_RECT GetScizzorRect() const
			{
				// CORRECT?
				D3D12_RECT ScizzorRect;
				ScizzorRect.left = static_cast<LONG>(m_TopLeft.x);
				ScizzorRect.top = static_cast<LONG>(m_TopLeft.y);
				ScizzorRect.right = static_cast<LONG>(m_TopLeft.x + m_Dimensions.x);
				ScizzorRect.bottom = static_cast<LONG>(m_TopLeft.y + m_Dimensions.y);
				return ScizzorRect;
			}

			DXM::Vector3 GetRight()
			{
				DXM::Vector3 Right = m_ForwardVector.Cross(m_UpVector);
				Right.Normalize();
				return Right;
			}

			float GetAspectRatio()
			{
				return m_Dimensions.x / m_Dimensions.y;
			}
		};
	}
}