#pragma once
#include "graphics_api/resource/ResourceBase.hpp"

namespace aZero
{
	namespace RenderAPI
	{
		class Texture2D : public ResourceBase
		{
		public:
			struct Desc
			{
				uint32_t Width = 0;
				uint32_t Height = 0;
				uint32_t MipLevels = 1;
				DXGI_FORMAT Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
				D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

				Desc() = default;
				Desc(uint32_t width, uint32_t height, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE)
					:Width(width), Height(height), Format(format), Flags(flags) {
				}
			};

			Texture2D() = default;

			// NOTE: If optClearValue is given during initialization the same value should be used during command list clear operations.The clear value can be accessed via Texture2D::GetClearValue().
			Texture2D(ID3D12DeviceX* device, const Desc& desc, std::optional<ResourceRecycler*> opt_diResourceRecycler = std::optional<ResourceRecycler*>{}, std::optional<D3D12_CLEAR_VALUE> optClearValue = std::optional<D3D12_CLEAR_VALUE>{});
			Texture2D(Texture2D&& other) noexcept;
			Texture2D& operator=(Texture2D&& other) noexcept;

			// NOTE: Texture2D::CreateTransition() will change the internal resource tracking. Once you've called this function you need to transition the resource to the returned state or else the tracking will be incorrect.
			D3D12_RESOURCE_BARRIER CreateTransition(D3D12_RESOURCE_STATES newState);

			std::optional<D3D12_CLEAR_VALUE> GetClearValue() const { return m_OptClearValue; }
			D3D12_RESOURCE_STATES GetState() const { return m_CurrentState; }
		private:
			D3D12_RESOURCE_STATES m_CurrentState = D3D12_RESOURCE_STATE_COMMON;
			std::optional<D3D12_CLEAR_VALUE> m_OptClearValue;
		};
	}
}