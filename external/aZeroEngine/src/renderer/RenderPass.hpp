#pragma once
#include "pipeline/pass/ComputeShaderPass.hpp"
#include "pipeline/pass/VertexShaderPass.hpp"
#include "pipeline/pass/MeshShaderPass.hpp"
#include <functional>

namespace aZero
{
	namespace Rendering
	{
		class PassBase
		{
		public:
			using OnExecuteCallback = std::function<void(RenderAPI::CommandList&, uint32_t)>;

			friend class Renderer;
			struct ConstantBinding
			{
				std::vector<uint8_t> Data;
				ConstantBinding() = default;

				template<typename T>
				ConstantBinding(const T& data)
				{
					Data.resize(sizeof(T));
					std::memcpy(Data.data(), &data, sizeof(T));
				}
			};

			struct TextureTransition
			{
				D3D12_RESOURCE_STATES State;
				RenderAPI::Texture2D* Texture;
			};

			struct DescriptionBase
			{
				std::unordered_map<std::string, ConstantBinding> Bindings;
				std::vector<TextureTransition> TextureTransitions;
				OnExecuteCallback StartCallback;
				OnExecuteCallback EndCallback;
				uint32_t ExecutionCount;
			};

			PassBase() = default;
			PassBase(std::unordered_map<std::string, ConstantBinding>&& bindings, std::vector<TextureTransition> textureTransitions, OnExecuteCallback startCallback, OnExecuteCallback endCallback, uint32_t executionCount)
				:m_Desc({std::move(bindings), std::move(textureTransitions), startCallback, endCallback, executionCount })
			{

			}

			virtual void Execute(RenderAPI::CommandQueue& cmdQueue, RenderAPI::CommandList& cmdList, const RenderAPI::DescriptorHeap& resourceHeap, const RenderAPI::DescriptorHeap& samplerHeap) const = 0;

		protected:
			void TransitionTextures(RenderAPI::CommandList& cmdList) const
			{
				std::vector<D3D12_RESOURCE_BARRIER> barriers;
				barriers.reserve(m_Desc.TextureTransitions.size());
				for (auto& transitions : m_Desc.TextureTransitions)
				{
					if (transitions.Texture->GetState() != transitions.State)
					{
						barriers.emplace_back(transitions.Texture->CreateTransition(transitions.State));
					}
				}

				if (barriers.size() > 0)
				{
					cmdList->ResourceBarrier(barriers.size(), barriers.data());
				}
			}

			void ExecuteCallbacks(RenderAPI::CommandList& cmdList) const
			{
				for (uint32_t i = 0; i < m_Desc.ExecutionCount; i++)
				{
					m_Desc.StartCallback(cmdList, i);
				}
			}

			const std::unordered_map<std::string, ConstantBinding>& GetBindings() const { return m_Desc.Bindings; }

		private:
			DescriptionBase m_Desc;
		};

		class Renderer; 

		class MeshShaderPass : public PassBase
		{
		public:
			friend class Renderer;

			struct InternalDesc
			{
				Pipeline::MeshShaderPass* Pipeline = nullptr; // Specific
				std::vector<Rendering::RenderTarget*> RenderTargets; // Mesh and Vert pipelines
				std::vector<bool> ClearRtvs; // Mesh and Vert pipelines
				Rendering::DepthStencilTarget* DepthStencilTarget; // Mesh and Vert pipelines
				bool ClearDsv; // Mesh and Vert pipelines
			};

			struct Description : public InternalDesc, public DescriptionBase { };

			MeshShaderPass() = default;

			MeshShaderPass(Description&& desc)
				:PassBase(std::move(desc.Bindings), std::move(desc.TextureTransitions), desc.StartCallback, desc.EndCallback, desc.ExecutionCount),
				m_DescInternal({desc.Pipeline, std::move(desc.RenderTargets), std::move(desc.ClearRtvs), desc.DepthStencilTarget, desc.ClearDsv})
			{

			}

			void Execute(RenderAPI::CommandQueue& cmdQueue, RenderAPI::CommandList& cmdList, const RenderAPI::DescriptorHeap& resourceHeap, const RenderAPI::DescriptorHeap& samplerHeap) const final override
			{
				std::array<ID3D12DescriptorHeap*, 2> heaps{ resourceHeap.Get(), samplerHeap.Get() };
				cmdList->SetDescriptorHeaps(heaps.size(), heaps.data());

				std::vector<RenderAPI::Descriptor*> rtvs;
				rtvs.reserve(m_DescInternal.RenderTargets.size());
				for (uint32_t i = 0; i < m_DescInternal.RenderTargets.size(); i++)
				{
					auto& rtv = m_DescInternal.RenderTargets[i];
					if (m_DescInternal.ClearRtvs[i])
					{
						if (rtv->GetTexture().GetState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
						{
							auto rtvBarrier = rtv->GetTexture().CreateTransition(D3D12_RESOURCE_STATE_RENDER_TARGET);
							cmdList->ResourceBarrier(1, &rtvBarrier);
						}
						cmdList->ClearRenderTargetView(rtv->GetCpuHandle(), rtv->GetClearValue().Color, 0, nullptr);
					}
					rtvs.emplace_back(&rtv->GetDescriptor());
				}

				auto& dsv = m_DescInternal.DepthStencilTarget;
				if (m_DescInternal.ClearDsv)
				{
					if (dsv->GetTexture().GetState() != D3D12_RESOURCE_STATE_DEPTH_WRITE)
					{
						auto dsvBarrier = dsv->GetTexture().CreateTransition(D3D12_RESOURCE_STATE_DEPTH_WRITE);
						cmdList->ResourceBarrier(1, &dsvBarrier);
					}
					cmdList->ClearDepthStencilView(dsv->GetCpuHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, dsv->GetClearValue().DepthStencil.Depth, dsv->GetClearValue().DepthStencil.Stencil, 0, nullptr);
				}

				this->TransitionTextures(cmdList);

				cmdQueue.ExecuteCommandList(cmdList, false);

				m_DescInternal.Pipeline->Begin(cmdList, resourceHeap, samplerHeap, rtvs, dsv != nullptr ? &dsv->GetDescriptor() : std::optional<RenderAPI::Descriptor*>());

				for (const auto& binding : PassBase::GetBindings())
				{
					auto rootBinding = m_DescInternal.Pipeline->GetConstantBindingIndex(binding.first);
					cmdList->SetGraphicsRoot32BitConstants(rootBinding.GetRootIndex(), std::ceil(binding.second.Data.size() / 4), binding.second.Data.data(), 0);
				}

				this->ExecuteCallbacks(cmdList);

				cmdQueue.ExecuteCommandList(cmdList, false);
			}

		private:
			InternalDesc m_DescInternal;
		};

		/*class ComputeShaderPass
		{
		public:
			ComputeShaderPass() = default;
		private:

		};*/

		// TODO: Impl
		/*class VertexShaderPass
		{
		public:
			VertexShaderPass() = default;
		private:

		};*/
	}
}