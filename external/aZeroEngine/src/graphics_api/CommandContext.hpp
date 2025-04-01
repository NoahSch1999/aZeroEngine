#pragma once
#include "graphics_api/D3D12Include.hpp"
#include "misc/NonCopyable.hpp"

#include <optional>

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

		class CommandContextAllocator : public NonCopyable
		{
		public:
			class CommandContextHandle : public NonCopyable
			{
				friend CommandContextAllocator;
			private:
				CommandContextAllocator* m_Parent = nullptr;
				uint32_t m_Index;

			public:
				CommandContextHandle(CommandContext* Context, uint32_t Index, CommandContextAllocator* Parent)
					:m_Context(Context), m_Index(Index), m_Parent(Parent) { }

				~CommandContextHandle()
				{
					if (m_Parent)
					{
						m_Parent->FreeContext(*this);
					}
				}

				CommandContextHandle(CommandContextHandle&& Other) noexcept
				{
					m_Parent = Other.m_Parent;
					m_Index = Other.m_Index;
					m_Context = Other.m_Context;
					Other.m_Parent = nullptr;
				}

				CommandContextHandle& operator=(CommandContextHandle&& Other) noexcept
				{
					if (this != &Other)
					{
						m_Parent = Other.m_Parent;
						m_Index = Other.m_Index;
						m_Context = Other.m_Context;
						Other.m_Parent = nullptr;
					}
					return *this;
				}

				CommandContext* m_Context;
			};

		private:
			std::vector<std::unique_ptr<CommandContext>> m_Contexts;
			std::vector<bool> m_IsLent;

		public:
			CommandContextAllocator() = default;

			CommandContextAllocator(ID3D12Device* Device, D3D12_COMMAND_LIST_TYPE Type, uint32_t NumContexts)
			{
				this->Init(Device, Type, NumContexts);
			}

			void Init(ID3D12Device* Device, D3D12_COMMAND_LIST_TYPE Type, uint32_t NumContexts)
			{
				m_Contexts.resize(NumContexts);
				m_IsLent.resize(NumContexts);
				for (int i = 0; i < m_Contexts.size(); i++)
				{
					m_Contexts.at(i) = std::make_unique<CommandContext>(Device, Type);
					m_IsLent.at(i) = false;
				}
			}

			std::optional<CommandContextHandle> GetContext()
			{
				for (int i = 0; i < m_Contexts.size(); i++)
				{
					if (!m_IsLent.at(i))
					{
						m_IsLent.at(i) = true;
						return CommandContextHandle(m_Contexts.at(i).get(), i, this);
					}
				}

				return {};
			}

			void FreeContext(CommandContextHandle& Handle)
			{
				if (m_IsLent.at(Handle.m_Index) && Handle.m_Parent == this)
				{
					m_IsLent.at(Handle.m_Index) = false;
					Handle.m_Parent = nullptr;
				}
			}

			void FreeCommandBuffers()
			{
				for (auto& CmdContext : m_Contexts)
				{
					CmdContext->FreeCommandBuffer();
				}
			}
		};
	}
}
