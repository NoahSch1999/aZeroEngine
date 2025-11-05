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
		// TODO: Rework
		enum class GRAPH_PASS_TYPE { Per_Object_Static };
		class RenderGraphPass
		{
		public:

			struct BufferBinding
			{
				std::string BindingName;
				Pipeline::SHADER_TYPE ShaderType;
				D3D12::GPUBuffer* Buffer = nullptr;
			};

			struct ConstantEntry
			{
				std::array<std::byte, 4> c = {};
			};

			struct RootConstant
			{
				std::string BindingName;
				Pipeline::SHADER_TYPE ShaderType;
				std::vector<ConstantEntry> Data;
			};

			Pipeline::RenderPass m_Pass;
			Pipeline::PASS_TYPE m_Type;
			GRAPH_PASS_TYPE m_GraphPassType;

			std::vector<BufferBinding> m_BufferBindings;
			std::vector<RootConstant> m_VSRootConstants;
			std::vector<RootConstant> m_PSRootConstants;
			std::vector<RootConstant> m_CSRootConstants;

			std::vector<const D3D12::RenderTargetView*> m_RTVBindings;
			std::optional<const D3D12::DepthStencilView*> m_DSVBinding;
			bool m_ShouldClearOnExecute = false;

		public:
			RenderGraphPass() = default;

			RenderGraphPass(Pipeline::RenderPass&& Pass, GRAPH_PASS_TYPE GraphPassType = GRAPH_PASS_TYPE::Per_Object_Static)
				:m_Pass(std::forward<Pipeline::RenderPass>(Pass)), m_GraphPassType(GraphPassType)
			{
				// Generate empty bindings based on pass reflection
				for (auto& [Key, Value] : m_Pass.m_VSResourceMap)
				{
					if (Value.m_ResourceType != D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
					{
						BufferBinding Binding;
						Binding.BindingName = Key;
						Binding.ShaderType = Pipeline::SHADER_TYPE::VS;
						m_BufferBindings.push_back(Binding);
					}
					else
					{
						RootConstant Constant;
						Constant.BindingName = Key;
						Constant.ShaderType = Pipeline::SHADER_TYPE::VS;
						for (int i = 0; i < Value.m_Num32BitConstants; i++)
						{
							Constant.Data.push_back(ConstantEntry());
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
						Binding.ShaderType = Pipeline::SHADER_TYPE::PS;
						m_BufferBindings.push_back(Binding);
					}
					else
					{
						RootConstant Constant;
						Constant.BindingName = Key;
						Constant.ShaderType = Pipeline::SHADER_TYPE::PS;
						for (int i = 0; i < Value.m_Num32BitConstants; i++)
						{
							Constant.Data.push_back(ConstantEntry());
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
						Binding.ShaderType = Pipeline::SHADER_TYPE::CS;
						m_BufferBindings.push_back(Binding);
					}
					else
					{
						RootConstant Constant;
						Constant.BindingName = Key;
						Constant.ShaderType = Pipeline::SHADER_TYPE::CS;
						for (int i = 0; i < Value.m_Num32BitConstants; i++)
						{
							Constant.Data.push_back(ConstantEntry());
						}
						m_CSRootConstants.push_back(Constant);
					}
				}

				m_RTVBindings.resize(m_Pass.m_NumRenderTargets);

				m_Type = m_Pass.GetPassType();
			}

			void BindBuffer(const std::string& ShaderName, Pipeline::SHADER_TYPE Type, D3D12::GPUBuffer* Buffer)
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

			void BindConstant(const std::string& ShaderName, Pipeline::SHADER_TYPE Type, void* Data, uint32_t NumBytes)
			{
				std::vector<RootConstant>* Constants = nullptr;
				if (Type == Pipeline::SHADER_TYPE::VS)
				{
					Constants = &m_VSRootConstants;
				}
				else if (Type == Pipeline::SHADER_TYPE::PS)
				{
					Constants = &m_PSRootConstants;
				}
				else if (Type == Pipeline::SHADER_TYPE::CS)
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
						uint32_t numBytesCopy = std::min((uint32_t)(RootConst.Data.size() * sizeof(ConstantEntry)), NumBytes);
						memcpy(RootConst.Data.data(), Data, numBytesCopy);
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

				if (m_Type == Pipeline::PASS_TYPE::GRAPHICS)
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
						DEBUG_PRINT("Forgot to bind: " + RootBinding.BindingName);
						return false;
					}
				}

				if (m_Type == Pipeline::PASS_TYPE::GRAPHICS)
				{
					if (m_GraphPassType == GRAPH_PASS_TYPE::Per_Object_Static)
					{
						for (auto& Constant : m_VSRootConstants)
						{
							m_Pass.SetShaderResource(CmdList,
								Constant.BindingName,
								Constant.Data.data(), Constant.Data.size() * sizeof(uint32_t),
								Pipeline::SHADER_TYPE::VS);
						}

						for (auto& Constant : m_PSRootConstants)
						{
							m_Pass.SetShaderResource(CmdList,
								Constant.BindingName,
								Constant.Data.data(), Constant.Data.size() * sizeof(uint32_t),
								Pipeline::SHADER_TYPE::PS);
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
							Pipeline::SHADER_TYPE::CS);
					}*/
					/*if (m_CSRootConstants.size())
					{
						m_Pass.SetShaderResource(CmdList, "RootConstants", m_CSRootConstants.data(), m_CSRootConstants.size(), Pipeline::SHADER_TYPE::CS);
					}*/
				}

				return true;
			}
		};
	}
}