#pragma once
#include "Core/D3D12Core.h"

namespace aZero
{
	namespace D3D12
	{
		class CommandContext
		{
		private:
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_allocator = nullptr;
			Microsoft::WRL::ComPtr<ID3D12CommandList> m_commandList = nullptr;

		public:
			CommandContext() = default;

			CommandContext(D3D12_COMMAND_LIST_TYPE type)
			{
				this->Init(type);
			}

			void Init(D3D12_COMMAND_LIST_TYPE type)
			{
				if (FAILED(gDevice->CreateCommandAllocator(type, IID_PPV_ARGS(m_allocator.GetAddressOf()))))
				{
					throw;
				}

				if (FAILED(gDevice->CreateCommandList(0, type, m_allocator.Get(), nullptr, IID_PPV_ARGS(m_commandList.GetAddressOf()))))
				{
					throw;
				}

				static_cast<ID3D12GraphicsCommandList*>(m_commandList.Get())->Close();
			}

			void StartRecording(ID3D12PipelineState* const startPSO = nullptr)
			{
				static_cast<ID3D12GraphicsCommandList*>(m_commandList.Get())->Reset(m_allocator.Get(), startPSO);
			}

			void StopRecording()
			{
				static_cast<ID3D12GraphicsCommandList*>(m_commandList.Get())->Close();
			}

			void FreeCommandBuffer()
			{
				m_allocator.Get()->Reset();
			}

			ID3D12GraphicsCommandList* const GetCommandList() const { return static_cast<ID3D12GraphicsCommandList*>(m_commandList.Get()); }
		};
	}
}
