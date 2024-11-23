#pragma once
#include "CommandContext.h"

namespace aZero
{
	namespace D3D12
	{
		class CommandQueue
		{
		private:
			Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_queue;
			Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
			D3D12_COMMAND_LIST_TYPE m_type = D3D12_COMMAND_LIST_TYPE_NONE;

			// NOTE - Issue if these are going above the valid range of UINT64? How should this be handles?
			UINT64 m_nextFenceValue = 0;
			UINT64 m_latestFenceValue = 0;

		public:
			CommandQueue() = default;

			CommandQueue(D3D12_COMMAND_LIST_TYPE type)
			{
				this->Init(type);
			}

			void Init(D3D12_COMMAND_LIST_TYPE type)
			{
				m_type = type;

				D3D12_COMMAND_QUEUE_DESC desc;
				desc.Type = type;
				desc.NodeMask = 0;
				desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
				desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

				if (FAILED(gDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(m_queue.GetAddressOf()))))
				{
					throw;
				}

				if (FAILED(gDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()))))
				{
					throw;
				}
			}

			ID3D12CommandQueue* const GetCommandQueue() const { return m_queue.Get(); }

			UINT64 GetLatestFenceValue() const { return m_latestFenceValue; }

			// TODO - Cleanup and try to make it more efficient by avoiding copy
			UINT64 ExecuteContext(const std::vector<CommandContext>& contexts)
			{
				std::vector<ID3D12CommandList*> lists;
				lists.reserve(contexts.size());

				for (const CommandContext& context : contexts)
				{
					lists.push_back(context.GetCommandList());
				}

				m_queue->ExecuteCommandLists(lists.size(), lists.data());

				m_queue->Signal(m_fence.Get(), m_nextFenceValue);
				m_latestFenceValue = m_nextFenceValue;
				m_nextFenceValue++;
				return m_latestFenceValue;
			}

			UINT64 ExecuteContextAfterSync(const std::vector<CommandContext>& contexts, const CommandQueue& otherQueue, UINT64 signalValue)
			{
				m_queue->Wait(otherQueue.m_fence.Get(), signalValue);

				std::vector<ID3D12CommandList*> lists;
				lists.reserve(contexts.size());

				for (const CommandContext& context : contexts)
				{
					lists.push_back(context.GetCommandList());
				}

				m_queue->ExecuteCommandLists(lists.size(), lists.data());

				m_queue->Signal(m_fence.Get(), m_nextFenceValue);
				m_latestFenceValue = m_nextFenceValue;
				m_nextFenceValue++;
				return m_latestFenceValue;
			}

			void FlushCommands()
			{
				UINT64 currentFenceValue = m_fence->GetCompletedValue();

				if (m_latestFenceValue <= currentFenceValue)
				{
					return;
				}

				m_fence->SetEventOnCompletion(m_latestFenceValue, nullptr);
			}

			void FlushImmediate()
			{
				this->ForceSignal();
				this->FlushCommands();
			}

			UINT64 ForceSignal()
			{
				m_queue->Signal(m_fence.Get(), m_nextFenceValue);
				m_latestFenceValue = m_nextFenceValue;
				m_nextFenceValue++;
				return m_latestFenceValue;
			}
		};
	}
}
