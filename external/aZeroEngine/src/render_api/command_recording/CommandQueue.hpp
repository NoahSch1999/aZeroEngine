#pragma once
#include "CommandList.hpp"

namespace aZero
{
	namespace RenderAPI
	{
		class CommandQueue : public NonCopyable
		{
		public:
			CommandQueue() = default;
			CommandQueue(ID3D12DeviceX* device, D3D12_COMMAND_LIST_TYPE type);

			CommandQueue(CommandQueue&& other) noexcept;
			CommandQueue& operator=(CommandQueue&& other) noexcept;

			uint64_t Signal();
			void Flush();
			void SignalAndFlush();
			bool WaitForSignal(uint64_t value, bool shouldStall);

			uint64_t ExecuteCommandList(CommandList& commandList, bool shouldSignal = true);
			uint64_t ExecuteCommandLists(const std::vector<CommandList*>& commandLists, bool shouldSignal = true);

			ID3D12CommandQueue* const Get() const { return m_Queue.Get(); }
			uint64_t GetLatestSignal() const { return m_LatestFenceValue; }
			D3D12_COMMAND_LIST_TYPE GetType() const { return m_Queue->GetDesc().Type; }
			bool IsInitiated() const { return m_Queue != nullptr; }
		private:
			Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_Queue = nullptr;
			Microsoft::WRL::ComPtr<ID3D12FenceX> m_Fence = nullptr;

			// NOTE - Issue if these are going above the valid range of UINT64. Should this be handled? Probably not
			uint64_t m_NextFenceValue = 0;
			uint64_t m_LatestFenceValue = 0;
		};
	}
}