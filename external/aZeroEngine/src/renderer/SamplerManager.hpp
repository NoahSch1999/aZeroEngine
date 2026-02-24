#pragma once
#include "graphics_api/D3D12Include.hpp"

namespace aZero
{
	namespace Rendering
	{
		struct SamplerManager
		{
			enum Type { Anisotropic_8x_Wrap = 0 };
			std::vector<RenderAPI::Descriptor> m_Descriptors;

			SamplerManager() = default;
			SamplerManager(ID3D12DeviceX* device, RenderAPI::DescriptorHeap& heap)
			{
				D3D12_SAMPLER_DESC SamplerDesc;
				SamplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
				SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
				SamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
				SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
				SamplerDesc.MipLODBias = 0.f;
				SamplerDesc.MaxAnisotropy = 8;
				SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NONE;
				SamplerDesc.MinLOD = 0;
				SamplerDesc.MaxLOD = 6;
				m_Descriptors.emplace_back(heap.CreateDescriptor());
				device->CreateSampler(&SamplerDesc, m_Descriptors.at(m_Descriptors.size() - 1).GetCpuHandle());
			}

			const RenderAPI::Descriptor& GetSampler(Type type) { return m_Descriptors.at(type); }
		};
	}
}