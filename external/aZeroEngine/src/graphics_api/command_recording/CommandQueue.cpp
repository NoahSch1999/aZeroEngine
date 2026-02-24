#include "CommandQueue.hpp"

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