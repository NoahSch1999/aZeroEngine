#include "CommandList.hpp"
#include "graphics_api/descriptor/DescriptorHeap.hpp"
#include <array>

aZero::RenderAPI::CommandList::CommandList(ID3D12DeviceX* device, D3D12_COMMAND_LIST_TYPE type)
{
	const HRESULT commandAllocRes = device->CreateCommandAllocator(type, IID_PPV_ARGS(m_Allocator.GetAddressOf()));
	if (FAILED(commandAllocRes))
	{
		throw std::invalid_argument("Failed to create command allocator");
	}


	const HRESULT commandListRes = device->CreateCommandList(0, type, m_Allocator.Get(), nullptr, IID_PPV_ARGS(m_CommandList.GetAddressOf()));
	if (FAILED(commandListRes))
	{
		throw std::invalid_argument("Failed to create command list");
	}

	m_Type = type;
}

void aZero::RenderAPI::CommandList::StartRecording()
{
	if (!m_IsRecording)
	{
		m_CommandList->Reset(m_Allocator.Get(), nullptr);
		m_IsRecording = true;
	}
}

void aZero::RenderAPI::CommandList::StopRecording()
{
	if (m_IsRecording)
	{
		m_CommandList->Close();
		m_IsRecording = false;
	}
}

aZero::RenderAPI::CommandList::CommandList(CommandList&& other) noexcept
{
	*this = std::move(other);
}

aZero::RenderAPI::CommandList& aZero::RenderAPI::CommandList::operator=(CommandList&& other) noexcept
{
	std::swap(m_CommandList, other.m_CommandList);
	std::swap(m_Allocator, other.m_Allocator);
	std::swap(m_IsRecording, other.m_IsRecording);
	std::swap(m_Type, other.m_Type);
	return *this;
}

void aZero::RenderAPI::CommandList::ClearCommandBuffer()
{
	this->StopRecording();
	m_Allocator->Reset();
	this->StartRecording();
}

void aZero::RenderAPI::CommandList::SetDescriptorHeaps(const RenderAPI::DescriptorHeap& resourceHeap, const RenderAPI::DescriptorHeap& samplerHeap)
{
	std::array<ID3D12DescriptorHeap*, 2> heaps{ resourceHeap.Get(), samplerHeap.Get() }; // Is having this local OK?
	this->Get()->SetDescriptorHeaps(heaps.size(), heaps.data());
}

void aZero::RenderAPI::CommandList::OMSetRenderTargets(const std::vector< std::reference_wrapper<RenderAPI::Descriptor>>& renderTargets, const std::optional<std::reference_wrapper<RenderAPI::Descriptor>>& depthStencilTarget)
{
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 8> outputTargets = {};
	for (uint32_t i = 0; i < renderTargets.size(); i++)
	{
		outputTargets[i] = renderTargets[i].get().GetCpuHandle();
	}

	if (depthStencilTarget.has_value())
	{
		D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptor = depthStencilTarget.value().get().GetCpuHandle();
		this->Get()->OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), outputTargets.size() > 0 ? outputTargets.data() : nullptr, 0, &dsvDescriptor);
	}
	else
	{
		this->Get()->OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), outputTargets.size() > 0 ? outputTargets.data() : nullptr, 0, nullptr);
	}
}
