#include "CommandList.hpp"

aZero::RenderAPI::CommandList::CommandList(ID3D12DeviceX* device, D3D12_COMMAND_LIST_TYPE type)
{
	if (!m_Allocator && !m_CommandList)
	{
		const HRESULT commandAllocRes = device->CreateCommandAllocator(type, IID_PPV_ARGS(m_Allocator.GetAddressOf()));
		if (FAILED(commandAllocRes))
		{
			throw std::invalid_argument("Failed to create command allocator");
		}

		m_Type = type;

		const HRESULT commandListRes = device->CreateCommandList(0, m_Type, m_Allocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandList));
		if (FAILED(commandListRes))
		{
			throw std::invalid_argument("Failed to create command list");
		}
	}
}

void aZero::RenderAPI::CommandList::Move(CommandList& other)
{
	m_CommandList = other.m_CommandList;
	m_Allocator = other.m_Allocator;
	m_IsRecording = other.m_IsRecording;
	m_Type = other.m_Type;
	other.m_CommandList = nullptr;
	other.m_Allocator = nullptr;
	other.m_IsRecording = false;
	other.m_Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_NONE;
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
	this->Move(other);
}

aZero::RenderAPI::CommandList& aZero::RenderAPI::CommandList::operator=(CommandList&& other) noexcept
{
	if (this != &other)
	{
		this->Move(other);
	}
	return *this;
}

void aZero::RenderAPI::CommandList::ClearCommandBuffer()
{
	m_CommandList->Close();
	m_Allocator->Reset();
}