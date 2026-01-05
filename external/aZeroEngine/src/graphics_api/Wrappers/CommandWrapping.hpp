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

		// todo Manage so command lists dont risk to reference a moved CommandListAllocator
		class CommandList : public NonCopyable
		{
			friend class CommandListAllocator;
		private:
			CommandListAllocator* m_Allocator = nullptr;
			ID3D12GraphicsCommandListX* m_CommandList = nullptr;
			bool m_IsRecording = true;

			CommandList(ID3D12GraphicsCommandListX* commandList, CommandListAllocator& allocator, D3D12_COMMAND_LIST_TYPE type);
			void Move(CommandList& other);

		public:
			ID3D12GraphicsCommandListX* operator->() { return m_CommandList; }
			const ID3D12GraphicsCommandListX* operator->() const { return m_CommandList; }

			void StartRecording();
			void StopRecording();

			CommandList() = default;
			~CommandList();
			CommandList(CommandList&& other) noexcept;
			CommandList& operator=(CommandList&& other) noexcept;

			D3D12_COMMAND_LIST_TYPE GetType() const { return m_CommandList->GetType(); }
			ID3D12GraphicsCommandListX* Get() const { return m_CommandList; }
			bool IsInitiated() const { return m_Allocator != nullptr; }
			bool IsRecording() const { return m_IsRecording; }
		};

		class CommandListAllocator : public NonCopyable
		{
		private:
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_Allocator = nullptr;
			D3D12_COMMAND_LIST_TYPE m_Type;
			std::set<ID3D12GraphicsCommandListX*> m_UsedCommandLists;
			std::stack<ID3D12GraphicsCommandListX*> m_FreeCommandLists;

		public:
			CommandListAllocator() = default;
			CommandListAllocator(ID3D12DeviceX* device, D3D12_COMMAND_LIST_TYPE type);

			void Init(ID3D12DeviceX* device, D3D12_COMMAND_LIST_TYPE type);

			CommandList CreateCommandList();
			void FreeCommandBuffer();
			void RecycleCommandList(CommandList& commandList);

			D3D12_COMMAND_LIST_TYPE GetType() const { return m_Type; }
			ID3D12CommandAllocator* const Get() const { return m_Allocator.Get(); }
			bool IsInitiated() const { return m_Allocator != nullptr; }
		};

		class CommandQueue : public NonCopyable
		{
		private:
			Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_Queue = nullptr;
			Microsoft::WRL::ComPtr<ID3D12FenceX> m_Fence = nullptr;
			D3D12_COMMAND_LIST_TYPE m_Type;

			// NOTE - Issue if these are going above the valid range of UINT64. Should this be handled? Probably not
			uint64_t m_NextFenceValue = 0;
			uint64_t m_LatestFenceValue = 0;

		public:
			CommandQueue() = default;
			CommandQueue(ID3D12DeviceX* device, D3D12_COMMAND_LIST_TYPE type);

			void Init(ID3D12DeviceX* device, D3D12_COMMAND_LIST_TYPE type);

			uint64_t Signal();
			void Flush();
			void SignalAndFlush();
			bool WaitForSignal(uint64_t value, bool shouldStall);

			uint64_t ExecuteCommandList(CommandList& commandList);
			uint64_t ExecuteCommandLists(const std::vector<CommandList*>& commandLists);

			ID3D12CommandQueue* const Get() const { return m_Queue.Get(); }
			uint64_t GetLatestSignal() const { return m_LatestFenceValue; }
			D3D12_COMMAND_LIST_TYPE GetType() const { return m_Type; }
			bool IsInitiated() const { return m_Queue != nullptr; }
		};
	}
}