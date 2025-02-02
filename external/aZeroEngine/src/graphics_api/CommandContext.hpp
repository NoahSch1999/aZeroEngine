#pragma once
#include "graphics_api/D3D12Include.hpp"
#include "misc/NonCopyable.hpp"

namespace aZero
{
	namespace D3D12
	{
		class CommandContext : public NonCopyable
		{
		private:
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_Allocator = nullptr;
			Microsoft::WRL::ComPtr<ID3D12CommandList> m_CommandList = nullptr;
			bool m_IsRecording;

		public:
			CommandContext() = default;

			CommandContext(CommandContext&& Other) noexcept
			{
				m_Allocator = std::move(Other.m_Allocator);
				m_CommandList = std::move(Other.m_CommandList);
				m_IsRecording = Other.m_IsRecording;
			}

			CommandContext& operator=(CommandContext&& Other) noexcept
			{
				if (this != &Other)
				{
					m_Allocator = std::move(Other.m_Allocator);
					m_CommandList = std::move(Other.m_CommandList);	
					m_IsRecording = Other.m_IsRecording;
				}
				return *this;
			}

			CommandContext(ID3D12Device* Device, D3D12_COMMAND_LIST_TYPE Type)
			{
				this->Init(Device, Type);
			}

			void Init(ID3D12Device* Device, D3D12_COMMAND_LIST_TYPE Type)
			{
				const HRESULT CommandAllocRes = Device->CreateCommandAllocator(Type, IID_PPV_ARGS(m_Allocator.GetAddressOf()));
				if (FAILED(CommandAllocRes))
				{
					throw std::invalid_argument("CommandContext::Init() => Failed to create command allcator");
				}

				const HRESULT CommandListRes = Device->CreateCommandList(0, Type, m_Allocator.Get(), nullptr, IID_PPV_ARGS(m_CommandList.GetAddressOf()));
				if (FAILED(CommandListRes))
				{
					throw std::invalid_argument("CommandContext::Init() => Failed to create command list");
				}

				m_IsRecording = true;
			}

			void StartRecording(ID3D12PipelineState* StartPipelineState = nullptr)
			{
				if (!m_IsRecording)
				{
					this->GetCommandList()->Reset(m_Allocator.Get(), StartPipelineState);
					m_IsRecording = true;
				}
				else if(StartPipelineState)
				{
					this->GetCommandList()->SetPipelineState(StartPipelineState);
				}
			}

			void StopRecording()
			{
				if (m_IsRecording)
				{
					static_cast<ID3D12GraphicsCommandList*>(m_CommandList.Get())->Close();
					m_IsRecording = false;
				}
			}

			void FreeCommandBuffer()
			{
				this->StopRecording();
				m_Allocator.Get()->Reset();
				this->StartRecording();
			}

			ID3D12GraphicsCommandList* GetCommandList() { return static_cast<ID3D12GraphicsCommandList*>(m_CommandList.Get()); }
		};
	}
}
