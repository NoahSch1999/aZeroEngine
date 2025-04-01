#pragma once
#include <variant>

#include "RenderPass.hpp"
#include "graphics_api/resource_type/GPUResource.hpp"
#include "graphics_api/resource_type/GPUResourceView.hpp"
#include "graphics_api/CommandContext.hpp"
#include "scene/Scene.hpp"

namespace aZero
{
	namespace Rendering
	{
		enum class GRAPH_PASS_TYPE { Per_Object_Static };
		class RenderGraphPass
		{
		public:

			struct BufferBinding
			{
				std::string BindingName;
				D3D12::SHADER_TYPE ShaderType;
				D3D12::GPUBuffer* Buffer = nullptr;
			};

			struct RootConstant
			{
				std::string BindingName;
				D3D12::SHADER_TYPE ShaderType;
				std::vector<int32_t> Data;
			};

			D3D12::RenderPass m_Pass;
			D3D12::PASS_TYPE m_Type;
			GRAPH_PASS_TYPE m_GraphPassType;

			std::vector<BufferBinding> m_BufferBindings;
			std::vector<RootConstant> m_VSRootConstants;
			std::vector<RootConstant> m_PSRootConstants;
			std::vector<RootConstant> m_CSRootConstants;

			std::vector<const D3D12::RenderTargetView*> m_RTVBindings;
			std::optional<const D3D12::DepthStencilView*> m_DSVBinding;
			bool m_ShouldClearOnExecute;

		public:
			RenderGraphPass() = default;

			RenderGraphPass(D3D12::RenderPass&& Pass, GRAPH_PASS_TYPE GraphPassType = GRAPH_PASS_TYPE::Per_Object_Static)
				:m_Pass(std::forward<D3D12::RenderPass>(Pass)), m_GraphPassType(GraphPassType)
			{
				// Generate empty bindings based on pass reflection
				for (auto& [Key, Value] : m_Pass.m_VSResourceMap)
				{
					if (Value.m_ResourceType != D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
					{
						BufferBinding Binding;
						Binding.BindingName = Key;
						Binding.ShaderType = D3D12::SHADER_TYPE::VS;
						m_BufferBindings.push_back(Binding);
					}
					else
					{
						RootConstant Constant;
						Constant.BindingName = Key;
						Constant.ShaderType = D3D12::SHADER_TYPE::VS;
						for (int i = 0; i < Value.m_Num32BitConstants; i++)
						{
							Constant.Data.push_back(0);
						}
						m_VSRootConstants.push_back(Constant);
					}
				}

				for (auto& [Key, Value] : m_Pass.m_PSResourceMap)
				{
					if (Value.m_ResourceType != D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
					{
						BufferBinding Binding;
						Binding.BindingName = Key;
						Binding.ShaderType = D3D12::SHADER_TYPE::PS;
						m_BufferBindings.push_back(Binding);
					}
					else
					{
						RootConstant Constant;
						Constant.BindingName = Key;
						Constant.ShaderType = D3D12::SHADER_TYPE::PS;
						for (int i = 0; i < Value.m_Num32BitConstants; i++)
						{
							Constant.Data.push_back(0);
						}
						m_PSRootConstants.push_back(Constant);
					}
				}

				for (auto& [Key, Value] : m_Pass.m_CSResourceMap)
				{
					if (Value.m_ResourceType != D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
					{
						BufferBinding Binding;
						Binding.BindingName = Key;
						Binding.ShaderType = D3D12::SHADER_TYPE::CS;
						m_BufferBindings.push_back(Binding);
					}
					else
					{
						RootConstant Constant;
						Constant.BindingName = Key;
						Constant.ShaderType = D3D12::SHADER_TYPE::CS;
						for (int i = 0; i < Value.m_Num32BitConstants; i++)
						{
							Constant.Data.push_back(0);
						}
						m_CSRootConstants.push_back(Constant);
					}
				}

				m_RTVBindings.resize(m_Pass.m_NumRenderTargets);

				m_Type = m_Pass.GetPassType();
			}

			void BindBuffer(const std::string& ShaderName, D3D12::SHADER_TYPE Type, D3D12::GPUBuffer* Buffer)
			{
				if (!Buffer)
				{
					throw std::runtime_error("Nullptr buffer bound");
				}

				for (auto& Binding : m_BufferBindings)
				{
					if ((Binding.BindingName == ShaderName) && (Binding.ShaderType == Type))
					{
						Binding.Buffer = Buffer;
						return;
					}
				}
			}

			void BindRenderTarget(uint32_t Slot, const D3D12::RenderTargetView* Binding)
			{
				if (!Binding)
				{
					throw std::runtime_error("Nullptr RenderTargetView bound");
				}

				if (m_RTVBindings.size() > Slot)
				{
					m_RTVBindings.at(Slot) = Binding;
				}
			}

			void BindDepthStencil(const D3D12::DepthStencilView* Binding)
			{
				if (!Binding)
				{
					throw std::runtime_error("Nullptr DepthStencilView bound");
				}

				if (m_Pass.m_HasDepthStencilTarget)
				{
					m_DSVBinding = Binding;
				}
			}

			void BindConstant(const std::string& ShaderName, D3D12::SHADER_TYPE Type, void* Data, uint32_t NumBytes)
			{
				std::vector<RootConstant>* Constants = nullptr;
				if (Type == D3D12::SHADER_TYPE::VS)
				{
					Constants = &m_VSRootConstants;
				}
				else if (Type == D3D12::SHADER_TYPE::PS)
				{
					Constants = &m_PSRootConstants;
				}
				else if (Type == D3D12::SHADER_TYPE::CS)
				{
					Constants = &m_CSRootConstants;
				}
				else
				{
					return;
				}

				for (auto& RootConst : *Constants)
				{
					if (RootConst.BindingName == ShaderName)
					{
						memcpy(RootConst.Data.data(), Data, std::min((uint32_t)RootConst.Data.size(), NumBytes));
					}
				}
			}

			bool BeginPass(ID3D12GraphicsCommandList* CmdList)
			{
				if (!m_Pass.GetPipelineState())
				{
					return false;
				}

				CmdList->SetPipelineState(m_Pass.GetPipelineState());

				if (m_Type == D3D12::PASS_TYPE::GRAPHICS)
				{
					CmdList->SetGraphicsRootSignature(m_Pass.GetRootSignature());
					const D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType = m_Pass.GetTopologyType();
					switch (TopologyType)
					{
					case D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE:
					{
						CmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
						break;
					}
					case D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE:
					{
						CmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_LINELIST);
						break;
					}
					case D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT:
					{
						CmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
						break;
					}
					default:
					{
						DEBUG_PRINT("Input pass has an invalid topology type");
					}
					}

					std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RTVHandles;
					for (const D3D12::RenderTargetView* Binding : m_RTVBindings)
					{
						if (!Binding)
						{
							return false;
						}

						RTVHandles.push_back(Binding->GetDescriptorHandle());

						if (m_ShouldClearOnExecute)
						{
							D3D12_CLEAR_VALUE RTVClearValue = D3D12_CLEAR_VALUE();
							const D3D12::GPUTexture* Texture = Binding->GetResource<D3D12::GPUTexture>();
							if (Texture->GetClearValue().has_value())
							{
								RTVClearValue = Texture->GetClearValue().value();
							}

							CmdList->ClearRenderTargetView(Binding->GetDescriptorHandle(), RTVClearValue.Color, 0, NULL);
						}
					}

					if (m_DSVBinding.has_value())
					{
						if (m_ShouldClearOnExecute)
						{
							D3D12_CLEAR_VALUE DSVClearValue = 
								m_DSVBinding.value()->GetResource<D3D12::GPUTexture>()->GetClearValue().has_value() 
								? 
								m_DSVBinding.value()->GetResource<D3D12::GPUTexture>()->GetClearValue().value() : D3D12_CLEAR_VALUE();

							CmdList->ClearDepthStencilView(
								m_DSVBinding.value()->GetDescriptorHandle(),
								D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
								DSVClearValue.DepthStencil.Depth, DSVClearValue.DepthStencil.Stencil,
								0, NULL);
						}

						D3D12_CPU_DESCRIPTOR_HANDLE TempHandle = m_DSVBinding.value()->GetDescriptorHandle();
						CmdList->OMSetRenderTargets(RTVHandles.size(), RTVHandles.data(), false, &TempHandle);
					}
					else
					{
						CmdList->OMSetRenderTargets(RTVHandles.size(), RTVHandles.data(), false, NULL);
					}
				}
				else
				{
					CmdList->SetComputeRootSignature(m_Pass.GetRootSignature());
				}

				for (const BufferBinding& RootBinding : m_BufferBindings)
				{
					if (RootBinding.Buffer && RootBinding.Buffer->GetResource())
					{
						m_Pass.SetShaderResource(CmdList, RootBinding.BindingName, RootBinding.Buffer->GetResource()->GetGPUVirtualAddress(), RootBinding.ShaderType);
					}
					else
					{
						return false;
					}
				}

				if (m_Type == D3D12::PASS_TYPE::GRAPHICS)
				{
					if (m_GraphPassType == GRAPH_PASS_TYPE::Per_Object_Static)
					{
						for (auto& Constant : m_VSRootConstants)
						{
							m_Pass.SetShaderResource(CmdList,
								Constant.BindingName,
								Constant.Data.data(), Constant.Data.size() * sizeof(uint32_t),
								D3D12::SHADER_TYPE::VS);
						}

						for (auto& Constant : m_PSRootConstants)
						{
							m_Pass.SetShaderResource(CmdList,
								Constant.BindingName,
								Constant.Data.data(), Constant.Data.size() * sizeof(uint32_t),
								D3D12::SHADER_TYPE::PS);
						}
					}
				}
				else
				{
					// TODO: Impl
					throw;
					/*for (auto& Constant : m_CSRootConstants)
					{
						m_Pass.SetShaderResource(CmdList,
							Constant.BindingName,
							Constant.Data.data(), Constant.Data.size() * sizeof(uint32_t),
							D3D12::SHADER_TYPE::CS);
					}*/
					/*if (m_CSRootConstants.size())
					{
						m_Pass.SetShaderResource(CmdList, "RootConstants", m_CSRootConstants.data(), m_CSRootConstants.size(), D3D12::SHADER_TYPE::CS);
					}*/
				}

				return true;
			}
		};

		class RenderGraph
		{
		private:
			D3D12::DescriptorHeap* m_ResourceHeap = nullptr;
			D3D12::DescriptorHeap* m_SamplerHeap = nullptr;

			std::list<RenderGraphPass*> m_Passes;
		private:

			void RenderStaticMeshes(RenderGraphPass& Pass,
				ID3D12GraphicsCommandList* CmdList,
				const Scene::Scene::Camera& Camera,
				const StaticMeshBatches& StaticMeshBatches,
				uint32_t NumDirectionalLights,
				uint32_t NumPointLights,
				uint32_t NumSpotLights
			)
			{
				CmdList->RSSetViewports(1, &Camera.Viewport);
				CmdList->RSSetScissorRects(1, &Camera.ScizzorRect);

				{
					struct PixelShaderConstants
					{
						uint32_t SamplerIndex;
					};

					struct VertexShaderConstants
					{
						DXM::Matrix ViewProjectionMatrix;
					} CameraConstants;
					CameraConstants.ViewProjectionMatrix = Camera.ViewMatrix * Camera.ProjMatrix;
					Pass.m_Pass.SetShaderResource(CmdList, "CameraDataBuffer", (void*)&CameraConstants, sizeof(CameraConstants), D3D12::SHADER_TYPE::VS);

					struct PerBatchConstantsVS
					{
						uint32_t StartInstanceOffset;
						uint32_t MeshEntryIndex;
					};

					struct PerBatchConstantsPS
					{
						uint32_t MaterialIndex;
						uint32_t NumDirectionalLights;
						uint32_t NumPointLights;
						uint32_t NumSpotLights;
					};

					PerBatchConstantsPS RootConstantsPS;
					RootConstantsPS.NumDirectionalLights = NumDirectionalLights;
					RootConstantsPS.NumPointLights = NumPointLights;
					RootConstantsPS.NumSpotLights = NumSpotLights;

					for (auto& [MaterialIndex, MeshToBatchMap] : StaticMeshBatches.Batches)
					{
						RootConstantsPS.MaterialIndex = MaterialIndex;
						for (auto& [MeshIndex, Batch] : MeshToBatchMap)
						{
							PerBatchConstantsVS VSConstants;
							VSConstants.MeshEntryIndex = MeshIndex;
							VSConstants.StartInstanceOffset = Batch.StartInstanceOffset;

							const uint32_t NumInstances = Batch.InstanceData.size();
							Pass.m_Pass.SetShaderResource(CmdList, "PerBatchConstantsBuffer", (void*)&VSConstants, sizeof(VSConstants), D3D12::SHADER_TYPE::VS);
							Pass.m_Pass.SetShaderResource(CmdList, "PerBatchConstantsBuffer", (void*)&RootConstantsPS, sizeof(PerBatchConstantsPS), D3D12::SHADER_TYPE::PS);

							CmdList->DrawInstanced(static_cast<UINT>(Batch.NumVertices), NumInstances, 0, 0);
						}
					}
				}
			}

		public:
			RenderGraph() = default;

			RenderGraph(D3D12::DescriptorHeap* ResourceDescHeap, D3D12::DescriptorHeap* SamplerDescHeap)
			{
				this->Init(ResourceDescHeap, SamplerDescHeap);
			}

			void Init(D3D12::DescriptorHeap* ResourceDescHeap, D3D12::DescriptorHeap* SamplerDescHeap)
			{
				m_ResourceHeap = ResourceDescHeap;
				m_SamplerHeap = SamplerDescHeap;
			}

			bool Execute(
				D3D12::CommandQueue& DirectQueue, 
				D3D12::CommandContext& Context, 
				const Scene::Scene::Camera& Camera,

				// Generated from the renderer/culling system for the input camera
				const Rendering::StaticMeshBatches& InStaticMeshBatches,
				uint32_t NumDirectionalLights,
				uint32_t NumPointLights,
				uint32_t NumSpotLights
				//
			)
			{
				for (RenderGraphPass* Pass : m_Passes)
				{
					ID3D12DescriptorHeap* Heaps[2] = { m_ResourceHeap->GetDescriptorHeap(), m_SamplerHeap->GetDescriptorHeap() };
					Context.GetCommandList()->SetDescriptorHeaps(2, Heaps);

					bool Succeeded = Pass->BeginPass(Context.GetCommandList());
					if (!Succeeded)
					{
						Context.FreeCommandBuffer();
						return false;
					}

					if (Pass->m_GraphPassType == GRAPH_PASS_TYPE::Per_Object_Static)
					{
						// Do path with input batches
						this->RenderStaticMeshes(*Pass, Context.GetCommandList(), Camera, InStaticMeshBatches, NumDirectionalLights, NumPointLights, NumSpotLights);
					}
					else
					{
						throw;
					}

					DirectQueue.ExecuteContext(Context);
				}

				return true;
			}

			void OptimizeDependencies()
			{
				// TODO: Impl

				//
			}

			void AddPass(RenderGraphPass& Pass, uint32_t PositionIndex)
			{
				// TODO: Impl
				m_Passes.emplace_back(&Pass);
				//

				this->OptimizeDependencies();
			}
		};
	}
}