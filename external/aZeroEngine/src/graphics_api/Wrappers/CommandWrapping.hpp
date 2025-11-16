#pragma once
#include <stack>
#include <vector>
#include <set>
#include "misc/NonCopyable.hpp"
#include "graphics_api/D3D12Include.hpp"

namespace aZero
{
	namespace RenderAPI
	{
		class CommandListAllocator;

		class CommandList : public NonCopyable
		{
			friend class CommandListAllocator;
		private:
			CommandListAllocator* m_Allocator = nullptr;
			ID3D12GraphicsCommandList* m_CommandList = nullptr;
			bool m_IsRecording;

			CommandList(ID3D12GraphicsCommandList* commandList, CommandListAllocator& allocator, D3D12_COMMAND_LIST_TYPE type)
				:m_Allocator(&allocator), m_CommandList(commandList), m_IsRecording(true) {}

			void Move(CommandList& other)
			{
				m_CommandList = other.m_CommandList;
				m_Allocator = other.m_Allocator;
				m_IsRecording = other.m_IsRecording;
				other.m_CommandList = nullptr;
				other.m_Allocator = nullptr;
			}

		public:
			ID3D12GraphicsCommandList* operator->() { return m_CommandList; }
			const ID3D12GraphicsCommandList* operator->() const { return m_CommandList; }

			void StartRecording()
			{
				if (!m_IsRecording)
				{
					m_CommandList->Reset(m_Allocator, nullptr);
					m_IsRecording = true;
				}
			}

			void StopRecording()
			{
				if (m_IsRecording)
				{
					m_CommandList.Get()->Close();
					m_IsRecording = false;
				}
			}

			~CommandList()
			{
				if (m_Allocator)
				{
					m_Allocator->RecycleCommandList(*this);
				}
			}

			CommandList(CommandList&& other) noexcept
			{
				this->Move(other);
			}

			CommandList& operator=(CommandList&& other) noexcept
			{
				if (this != &other)
				{
					this->Move(other);
				}
				return *this;
			}

			D3D12_COMMAND_LIST_TYPE GetType() const 
			{ 
				return m_CommandList->GetType();
			}

			ID3D12GraphicsCommandList* Get() const { return m_CommandList; }
			bool IsInitiated() const { return m_Allocator != nullptr; }
		};

		class CommandListAllocator : public NonCopyable
		{
		private:
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_Allocator = nullptr;
			D3D12_COMMAND_LIST_TYPE m_Type;
			std::set<ID3D12GraphicsCommandList*> m_UsedCommandLists;
			std::stack<ID3D12GraphicsCommandList*> m_FreeCommandLists;

			void Move(CommandListAllocator& other)
			{
				m_Allocator = std::move(other.m_Allocator);
				m_Type = other.m_Type;
				m_UsedCommandLists = std::move(other.m_UsedCommandLists);
				m_FreeCommandLists = std::move(other.m_FreeCommandLists);
				other.m_Allocator = nullptr;
			}
		public:
			CommandListAllocator() = default;

			CommandListAllocator(CommandListAllocator&& other) noexcept
			{
				this->Move(other);
			}

			CommandListAllocator& operator=(CommandListAllocator&& other) noexcept
			{
				if (this != &other)
				{
					this->Move(other);
				}
				return *this;
			}

			CommandListAllocator(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
			{
				this->Init(device, type);
			}

			void Init(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
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

			void FreeCommandBuffer()
			{
				m_Allocator.Get()->Reset();
				for (ID3D12GraphicsCommandList* commandList : m_UsedCommandLists)
				{
					commandList->Close();
				}

				m_Allocator.Get()->Reset();

				for (ID3D12GraphicsCommandList* commandList : m_UsedCommandLists)
				{
					commandList->Reset(m_Allocator, nullptr);
				}
			}

			CommandList CreateCommandList()
			{
				ID3D12GraphicsCommandList* commandList;
				if (!m_FreeCommandLists.empty())
				{
					commandList = m_FreeCommandLists.top();
					m_FreeCommandLists.pop();
					return CommandList(commandList, *this, m_Type);
				}

				ID3D12Device* device;
				const HRESULT getDeviceRes = m_Allocator->GetDevice(IID_PPV_ARGS(&device));
				if (FAILED(getDeviceRes))
				{
					throw std::invalid_argument("Failed to get command allocator device");
				}

				const HRESULT commandListRes = device->CreateCommandList(0, m_Type, m_Allocator.Get(), nullptr, IID_PPV_ARGS(m_CommandList.GetAddressOf()));
				if (FAILED(commandListRes))
				{
					throw std::invalid_argument("CommandContext::Init() => Failed to create command list");
				}
				return CommandList(commandList, *this, m_Type); // Thank you copy elision :)
			}

			void RecycleCommandList(CommandList& commandList)
			{
				if (commandList.m_Allocator && commandList.m_CommandList)
				{
					m_FreeCommandLists.push(commandList.m_CommandList);
					commandList.m_Allocator = nullptr;
					commandList.m_CommandList = nullptr;
				}
			}

			D3D12_COMMAND_LIST_TYPE GetType() const { return m_Type; }
			ID3D12CommandAllocator* const Get() const { return m_Allocator.Get(); }
			bool IsInitiated() const { return m_Allocator != nullptr; }
		};

		class CommandQueue : public NonCopyable
		{
		private:
			Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_Queue = nullptr;
			Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence = nullptr;
			D3D12_COMMAND_LIST_TYPE m_Type;

			// NOTE - Issue if these are going above the valid range of UINT64. Should this be handled?
			uint64_t m_NextFenceValue;
			uint64_t m_LatestFenceValue;

			void Move(CommandQueue& other)
			{
				m_Queue = std::move(other.m_Queue);
				other.m_Queue = nullptr;
				m_Fence = std::move(other.m_Fence);
				other.m_Fence = nullptr;
				m_Type = other.m_Type;
				m_NextFenceValue = other.m_NextFenceValue;
				m_LatestFenceValue = other.m_LatestFenceValue;
			}

		public:
			CommandQueue()
				:m_NextFenceValue(0), m_LatestFenceValue(0) {}

			CommandQueue(CommandQueue&& other) noexcept
			{
				this->Move(other);
			}

			CommandQueue& operator=(CommandQueue&& other) noexcept
			{
				if (this != &other)
				{
					this->Move(other);
				}
				return *this;
			}

			CommandQueue(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
			{
				this->Init(device, type);
			}

			void Init(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
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

			uint64_t Signal()
			{
				m_Queue->Signal(m_Fence.Get(), m_NextFenceValue);
				m_LatestFenceValue = m_NextFenceValue;
				m_NextFenceValue++;
				return m_LatestFenceValue;
			}

			void Flush()
			{
				const uint64_t CurrentFenceValue = m_Fence->GetCompletedValue();

				if (m_LatestFenceValue <= CurrentFenceValue)
				{
					return;
				}

				m_Fence->SetEventOnCompletion(m_LatestFenceValue, nullptr);
			}

			void SignalAndFlush()
			{
				this->Signal();
				this->Flush();
			}

			uint64_t ExecuteContext(CommandList& commandList)
			{
				commandList.StopRecording();
				m_Queue->ExecuteCommandLists(1, commandList.Get());
				commandList.StartRecording();
				return this->ForceSignal();
			}

			uint64_t ExecuteContexts(const std::vector<CommandList*>& commandLists)
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

			ID3D12CommandQueue* const Get() const { return m_Queue.Get(); }
			uint64_t GetLatestSignal() const { return m_LatestFenceValue; }
			D3D12_COMMAND_LIST_TYPE GetType() const { return m_Type; }
			bool IsInitiated() const { return m_Queue != nullptr; }
		};
	}
}