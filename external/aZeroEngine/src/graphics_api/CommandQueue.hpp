#pragma once
#include "CommandContext.hpp"

namespace aZero
{
	namespace D3D12
	{
		class CommandQueue : public NonCopyable
		{
		private:
			Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_Queue;
			Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;

			// NOTE - Issue if these are going above the valid range of UINT64? How should this be handled?
			uint64_t m_NextFenceValue = 0;
			uint64_t m_LatestFenceValue = 0;

		public:
			CommandQueue() = default;

			CommandQueue(CommandQueue&& Other) noexcept
			{
				m_Queue = std::move(Other.m_Queue);
				m_Fence = std::move(Other.m_Fence);
				m_NextFenceValue = Other.m_NextFenceValue;
				m_LatestFenceValue = Other.m_LatestFenceValue;
			}

			CommandQueue& operator=(CommandQueue&& Other) noexcept
			{
				if (this != &Other)
				{
					m_Queue = std::move(Other.m_Queue);
					m_Fence = std::move(Other.m_Fence);
					m_NextFenceValue = Other.m_NextFenceValue;
					m_LatestFenceValue = Other.m_LatestFenceValue;
				}
				return *this;
			}

			CommandQueue(ID3D12Device* Device, D3D12_COMMAND_LIST_TYPE Type)
			{
				this->Init(Device, Type);
			}

			void Init(ID3D12Device* Device, D3D12_COMMAND_LIST_TYPE Type)
			{
				D3D12_COMMAND_QUEUE_DESC Desc;
				Desc.Type = Type;
				Desc.NodeMask = 0;
				Desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
				Desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

				const HRESULT CommandQueueRes = Device->CreateCommandQueue(&Desc, IID_PPV_ARGS(m_Queue.GetAddressOf()));
				if (FAILED(CommandQueueRes))
				{
					throw std::invalid_argument("CommandQueue::Init() => Failed to create command queue");
				}

				const HRESULT FenceRes = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_Fence.GetAddressOf()));
				if (FAILED(FenceRes))
				{
					throw std::invalid_argument("CommandQueue::Init() => Failed to create command queue fence");
				}
			}

			UINT64 ForceSignal()
			{
				m_Queue->Signal(m_Fence.Get(), m_NextFenceValue);
				m_LatestFenceValue = m_NextFenceValue;
				m_NextFenceValue++;
				return m_LatestFenceValue;
			}

			void FlushCommands()
			{
				uint64_t CurrentFenceValue = m_Fence->GetCompletedValue();

				if (m_LatestFenceValue <= CurrentFenceValue)
				{
					return;
				}

				m_Fence->SetEventOnCompletion(m_LatestFenceValue, nullptr);
			}

			void FlushImmediate()
			{
				this->ForceSignal();
				this->FlushCommands();
			}

			uint64_t ExecuteContext(CommandContext& Context)
			{
				Context.StopRecording();
				ID3D12CommandList* CmdList = (ID3D12CommandList*)Context.GetCommandList();
				m_Queue->ExecuteCommandLists(1, &CmdList);
				Context.StartRecording();
				return this->ForceSignal();
			}

			uint64_t ExecuteContexts(const std::vector<CommandContext*>& Contexts)
			{
				std::vector<ID3D12CommandList*> Lists;
				Lists.reserve(Contexts.size());

				for (CommandContext* Context : Contexts)
				{
					Lists.push_back(Context->GetCommandList());
				}

				m_Queue->ExecuteCommandLists(static_cast<UINT>(Lists.size()), Lists.data());

				return this->ForceSignal();
			}

			// TODO: Check if this works...
			UINT64 ExecuteContextsAfterSync(const std::vector<CommandContext*>& Contexts, const CommandQueue& OtherQueue, UINT64 SignalValue)
			{
				m_Queue->Wait(OtherQueue.m_Fence.Get(), SignalValue);
				return this->ExecuteContexts(Contexts);
			}

			ID3D12CommandQueue* const GetCommandQueue() const { return m_Queue.Get(); }
			uint64_t GetLatestFenceValue() const { return m_LatestFenceValue; }
		};
	}
}
