#pragma once
#include "misc/NonCopyable.hpp"
#include "graphics_api/D3D12Include.hpp"

namespace aZero
{
	namespace RenderAPI
	{
		class CommandList : public NonCopyable
		{
		public:
			CommandList() = default;
			CommandList(ID3D12DeviceX* device, D3D12_COMMAND_LIST_TYPE type);

			CommandList(CommandList&& other) noexcept;
			CommandList& operator=(CommandList&& other) noexcept;

			ID3D12GraphicsCommandListX* operator->() { return m_CommandList.Get(); }
			const ID3D12GraphicsCommandListX* operator->() const { return m_CommandList.Get(); }

			void ClearCommandBuffer();

			void StartRecording();
			void StopRecording();

			D3D12_COMMAND_LIST_TYPE GetType() const { return m_CommandList->GetType(); }
			ID3D12GraphicsCommandListX* Get() const { return m_CommandList.Get(); }
			bool IsInitiated() const { return m_Allocator != nullptr; }
			bool IsRecording() const { return m_IsRecording; }
		private:
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_Allocator = nullptr;
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandListX> m_CommandList = nullptr;
			D3D12_COMMAND_LIST_TYPE m_Type = D3D12_COMMAND_LIST_TYPE_NONE;
			bool m_IsRecording = true;
		};
	}
}