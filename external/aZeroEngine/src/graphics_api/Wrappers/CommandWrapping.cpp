#include "CommandWrapping.hpp"

aZero::RenderAPI::CommandList::CommandList(ID3D12GraphicsCommandListX* commandList, CommandListAllocator& allocator, D3D12_COMMAND_LIST_TYPE type)
	:m_Allocator(&allocator), m_CommandList(commandList) {}

void aZero::RenderAPI::CommandList::Move(CommandList& other)
{
	m_CommandList = other.m_CommandList;
	m_Allocator = other.m_Allocator;
	m_IsRecording = other.m_IsRecording;
	other.m_CommandList = nullptr;
	other.m_Allocator = nullptr;
}

void aZero::RenderAPI::CommandList::StartRecording()
{
	if (!m_IsRecording)
	{
		m_CommandList->Reset(m_Allocator->Get(), nullptr);
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

aZero::RenderAPI::CommandList::~CommandList()
{
	if (m_Allocator)
	{
		m_Allocator->RecycleCommandList(*this);
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

aZero::RenderAPI::CommandListAllocator::CommandListAllocator(ID3D12DeviceX* device, D3D12_COMMAND_LIST_TYPE type)
{
	this->Init(device, type);
}

void aZero::RenderAPI::CommandListAllocator::Init(ID3D12DeviceX* device, D3D12_COMMAND_LIST_TYPE type)
{
	if (!m_Allocator)
	{
		const HRESULT commandAllocRes = device->CreateCommandAllocator(type, IID_PPV_ARGS(m_Allocator.GetAddressOf()));
		if (FAILED(commandAllocRes))
		{
			throw std::invalid_argument("CommandContext::Init() => Failed to create command allcator");
		}

		m_Type = type;
	}
}

void aZero::RenderAPI::CommandListAllocator::FreeCommandBuffer()
{
	m_Allocator.Get()->Reset();
	for (ID3D12GraphicsCommandListX* commandList : m_UsedCommandLists)
	{
		commandList->Close();
	}

	m_Allocator.Get()->Reset();

	for (ID3D12GraphicsCommandListX* commandList : m_UsedCommandLists)
	{
		commandList->Reset(m_Allocator.Get(), nullptr);
	}
}

aZero::RenderAPI::CommandList aZero::RenderAPI::CommandListAllocator::CreateCommandList()
{
	ID3D12GraphicsCommandListX* commandList;
	if (!m_FreeCommandLists.empty())
	{
		commandList = m_FreeCommandLists.top();
		m_FreeCommandLists.pop();
		return CommandList(commandList, *this, m_Type); // Thank you copy elision :)
	}

	ID3D12DeviceX* device;
	const HRESULT getDeviceRes = m_Allocator->GetDevice(IID_PPV_ARGS(&device));
	if (FAILED(getDeviceRes))
	{
		throw std::invalid_argument("Failed to get command allocator device");
	}

	const HRESULT commandListRes = device->CreateCommandList(0, m_Type, m_Allocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	if (FAILED(commandListRes))
	{
		throw std::invalid_argument("CommandContext::Init() => Failed to create command list");
	}
	return CommandList(commandList, *this, m_Type); // Thank you copy elision :)
}

void aZero::RenderAPI::CommandListAllocator::RecycleCommandList(CommandList& commandList)
{
	if (commandList.m_Allocator && commandList.m_CommandList)
	{
		m_FreeCommandLists.push(commandList.m_CommandList);
		commandList.m_Allocator = nullptr;
		commandList.m_CommandList = nullptr;
	}
}

aZero::RenderAPI::CommandQueue::CommandQueue(ID3D12DeviceX* device, D3D12_COMMAND_LIST_TYPE type)
{
	this->Init(device, type);
}

void aZero::RenderAPI::CommandQueue::Init(ID3D12DeviceX* device, D3D12_COMMAND_LIST_TYPE type)
{
	if (!m_Queue)
	{
		D3D12_COMMAND_QUEUE_DESC desc;
		desc.Type = type;
		desc.NodeMask = 0;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

		const HRESULT commandQueueRes = device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_Queue.GetAddressOf()));
		if (FAILED(commandQueueRes))
		{
			throw std::invalid_argument("CommandQueue::Init() => Failed to create command queue");
		}

		const HRESULT fenceRes = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_Fence.GetAddressOf()));
		if (FAILED(fenceRes))
		{
			throw std::invalid_argument("CommandQueue::Init() => Failed to create command queue fence");
		}

		m_Type = type;
	}
}

uint64_t aZero::RenderAPI::CommandQueue::Signal()
{
	m_Queue->Signal(m_Fence.Get(), m_NextFenceValue);
	m_LatestFenceValue = m_NextFenceValue;
	m_NextFenceValue++;
	return m_LatestFenceValue;
}

void aZero::RenderAPI::CommandQueue::Flush()
{
	const uint64_t CurrentFenceValue = m_Fence->GetCompletedValue();

	if (m_LatestFenceValue <= CurrentFenceValue)
	{
		return;
	}

	m_Fence->SetEventOnCompletion(m_LatestFenceValue, nullptr);
}

void aZero::RenderAPI::CommandQueue::SignalAndFlush()
{
	this->Signal();
	this->Flush();
}

uint64_t aZero::RenderAPI::CommandQueue::ExecuteCommandList(aZero::RenderAPI::CommandList& commandList)
{
	commandList.StopRecording();
	ID3D12CommandList* temp = static_cast<ID3D12CommandList*>(commandList.Get());
	m_Queue->ExecuteCommandLists(1, &temp);
	commandList.StartRecording();
	return this->Signal();
}

uint64_t aZero::RenderAPI::CommandQueue::ExecuteCommandLists(const std::vector<aZero::RenderAPI::CommandList*>& commandLists)
{
	std::vector<ID3D12CommandList*> lists;
	lists.reserve(commandLists.size());

	for (CommandList* commandList : commandLists)
	{
		commandList->StopRecording();
		lists.push_back(commandList->Get());
	}

	m_Queue->ExecuteCommandLists(lists.size(), lists.data());

	for (CommandList* commandList : commandLists)
	{
		commandList->StartRecording();
	}

	return this->Signal();
}

bool aZero::RenderAPI::CommandQueue::WaitForSignal(uint64_t value, bool shouldStall)
{
	const uint64_t CurrentFenceValue = m_Fence->GetCompletedValue();

	// If value has been reached => Return early with true
	if (value <= CurrentFenceValue)
	{
		return true;
	}

	// If value has NOT been reached => Return early with false or stall until reached if shouldStall is true
	if (shouldStall)
	{
		m_Fence->SetEventOnCompletion(value, nullptr);
		return true;
	}

	return false;
}