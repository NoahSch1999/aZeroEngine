#pragma once
#include "RenderPass_New.hpp"
#include "assets/Asset.hpp"
#include "graphics_api/CommandQueue.hpp"
#include <optional>
#include "misc/LinearAllocator.hpp"
#include "renderer/RenderSurface.hpp"
#include "MeshShader.hpp"
#include <span>

namespace aZero
{
	namespace Pipeline
	{
		class ScenePass : public RenderPass_New
		{
		public:
			struct StaticMeshInstanceData
			{
				DXM::Matrix Transform;
			};

			struct StaticMeshBatchDrawData
			{
				std::vector<StaticMeshInstanceData> InstanceData;
				uint32_t NumVertices;
				uint32_t BatchOffset;
				Asset::RenderID MeshIndex;
				Asset::RenderID MaterialIndex;
			};

			struct LightDrawData
			{
				uint32_t NumPointLights;
				uint32_t NumSpotLights;
				uint32_t NumDirectionalLights;
			};
			//

			enum class TOPOLOGY_TYPE { INVALID = 0, POINT = 1, LINE = 2, TRIANGLE = 3 };

			// todo Maybe throw away once initialized?
			struct PassDescription
			{
				struct RenderTarget
				{
					DXGI_FORMAT Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
					std::string Name;
				};

				struct DepthStencilTarget
				{
					DXGI_FORMAT Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
				};

				TOPOLOGY_TYPE TopologyType = TOPOLOGY_TYPE::INVALID;
				std::vector<RenderTarget> RenderTargets;
				DepthStencilTarget DepthStencil;
			};

			bool IsCompiled() { return m_PipelineState != nullptr; }

			void BindVertexShaderBuffer(const std::string& name, std::weak_ptr<D3D12::GPUBuffer> buffer)
			{
				if (auto it = m_Bindings.VSBuffers.find(name); it != m_Bindings.VSBuffers.end())
				{
					it->second.Buffer = buffer;
				}
				else
				{
					DEBUG_PRINT("No buffer in vertex shader found with name || " + name + " || found in ScenePass.");
				}
			}

			void BindPixelShaderBuffer(const std::string& name, std::weak_ptr<D3D12::GPUBuffer> buffer)
			{
				if (auto it = m_Bindings.PSBuffers.find(name); it != m_Bindings.PSBuffers.end())
				{
					it->second.Buffer = buffer;
				}
				else
				{
					DEBUG_PRINT("No buffer in pixel shader found with name || " + name + " || found in ScenePass.");
				}
			}

			void BindVertexShaderConstants(const std::string& name, void* data)
			{
				if (auto it = m_Bindings.VSRootConstants.find(name); it != m_Bindings.VSRootConstants.end())
				{
					memcpy(it->second.Data.get(), data, it->second.BindingInfo.m_Num32BitConstants * sizeof(int32_t));
				}
				else
				{
					DEBUG_PRINT("No root constant in vertex shader with name || " + name + " || found in ScenePass.");
				}
			}

			void BindPixelShaderConstants(const std::string& name, void* data)
			{
				if (auto it = m_Bindings.PSRootConstants.find(name); it != m_Bindings.PSRootConstants.end())
				{
					memcpy(it->second.Data.get(), data, it->second.BindingInfo.m_Num32BitConstants * sizeof(int32_t));
				}
				else
				{
					DEBUG_PRINT("No root constant in pixel shader with name || " + name + " || found in ScenePass.");
				}
			}

			void BindRenderTarget(const std::string& name, std::weak_ptr<Rendering::RenderSurface> renderTargetSurface)
			{
				if (renderTargetSurface.expired() || renderTargetSurface.lock()->GetType() != Rendering::RenderSurface::Type::Color_Target)
				{
					DEBUG_PRINT("Render target surface expired or non-color target");
					return;
				}

				if (auto it = m_Bindings.RenderTargets.find(name); it != m_Bindings.RenderTargets.end())
				{
					it->second = renderTargetSurface;
				}
				else
				{
					DEBUG_PRINT("No render target with name || " + name + " || found in ScenePass.");
				}
			}

			void BindDepthStencilTarget(std::weak_ptr<Rendering::RenderSurface> renderDepthStencilSurface)
			{
				if (renderDepthStencilSurface.expired() || renderDepthStencilSurface.lock()->GetType() != Rendering::RenderSurface::Type::Depth_Target)
				{
					DEBUG_PRINT("Render depth stencil surface expired or non-color target");
					return;
				}

				m_Bindings.DepthStencilTarget = renderDepthStencilSurface;
			}

			bool Compile(ID3D12Device* device, const PassDescription& description, std::weak_ptr<Pipeline::VertexShader> vertexShader, std::optional<std::weak_ptr<Pipeline::PixelShader>> pixelShader)
			{
				this->Reset();

				if (!this->ValidateAndAsignPass(description, vertexShader, pixelShader))
				{
					this->Reset();
					DEBUG_PRINT("Validation of ScenePass failed.");
					return false;
				}

				if (!this->CompilePassPipeline(device, description))
				{
					this->Reset();
					DEBUG_PRINT("Failed to compile ScenePass.");
					return false;
				}

				if (!this->GenerateBindings(description))
				{
					this->Reset();
					DEBUG_PRINT("Failed to generate bindings for ScenePass.");
					return false;
				}

				return true;
			}

			/*
				The Renderer:
					Performs culling
					Sort meshes etc. into batches
					Upload batches to the gpu
				The RenderGraph takes them as inputs and forward relevant data to relevant RenderPass's.
				The RenderPass performs command recording and execution.
			*/
			bool Execute(D3D12::CommandQueue& graphicsQueue, 
				D3D12::CommandContextAllocator::CommandContextHandle& cmdContext,
				ID3D12DescriptorHeap* resourceHeap,
				ID3D12DescriptorHeap* samplerHeap,
				const Scene::SceneProxy::Camera& camera, 
				aZero::LinearAllocator<>& staticMeshAllocator,
				const std::unordered_map<uint32_t, std::unordered_map<uint32_t, StaticMeshBatchDrawData>>& batches, 
				const LightDrawData& lightDrawData)
			{
				ID3D12GraphicsCommandList* cmdList = cmdContext.m_Context->GetCommandList();

				// todo We dont wanna skip rendering if valid dsv/rtv but the one not used is expired etc
				if (!this->ValidateBoundRenderSurfaces())
				{
					return false;
				}

				/*if (m_Bindings.DepthStencilTarget.has_value() && m_Bindings.DepthStencilTarget->Descriptor.expired())
				{
					DEBUG_PRINT("Depth stencil has expired.");
					return false;
				}*/


				std::vector<D3D12::ResourceTransitionBundles> transitionBarriers;
				for (auto& [descriptorIndex, texture] : m_Bindings.TextureBindings)
				{
					transitionBarriers.push_back({ D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, m_Bindings.DepthStencilTarget->lock()->GetTexture().GetResource() });
				}
				transitionBarriers.push_back({ D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, m_Bindings.DepthStencilTarget->lock()->GetTexture().GetResource() });

				std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles;
				for (auto& [name, rtv] : m_Bindings.RenderTargets)
				{
					/*if (rtv.Descriptor.expired())
					{
						DEBUG_PRINT("Render target with name || " + name + " || has expired.");
						return false;
					}*/

					transitionBarriers.push_back({ D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET, rtv.lock()->GetTexture().GetResource() });

					rtvHandles.emplace_back(rtv.lock()->GetView<D3D12::RenderTargetView>().GetDescriptorHandle());
				}

				D3D12::TransitionResources(cmdList, transitionBarriers);

				for (auto& [name, rtv] : m_Bindings.RenderTargets)
				{
					/*if (rtv.Descriptor.expired())
					{
						DEBUG_PRINT("Render target with name || " + name + " || has expired.");
						return false;
					}*/
					rtv.lock()->RecordClear(cmdList);
				}

				if (m_Bindings.DepthStencilTarget->lock()->ShouldClear())
				{
					m_Bindings.DepthStencilTarget->lock()->RecordClear(cmdList);
				}

				// todo Make batches into a simple vector which can be iterated
				// todo Multithread?
				for (const auto& [meshIndex, batchArrayMap] : batches)
				{
					for (const auto& [materialIndex, batchArray] : batchArrayMap)
					{
						LinearAllocator<>::Allocation batchAllocation = staticMeshAllocator.Append((void*)batchArray.InstanceData.data(), batchArray.InstanceData.size() * sizeof(decltype(batchArray.InstanceData)::value_type));
						this->RenderBatch(cmdList, resourceHeap, samplerHeap, rtvHandles, camera, batchAllocation.Offset, batchArray, lightDrawData);
						graphicsQueue.ExecuteContext(*cmdContext.m_Context);
					}
				}

				transitionBarriers.clear();
				transitionBarriers.push_back({ D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COMMON, m_Bindings.DepthStencilTarget->lock()->GetTexture().GetResource() });

				for (auto& [name, rtv] : m_Bindings.RenderTargets)
				{
					/*if (rtv.Descriptor.expired())
					{
						DEBUG_PRINT("Render target with name || " + name + " || has expired.");
						return false;
					}*/
					transitionBarriers.push_back({ D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON, rtv.lock()->GetTexture().GetResource()});
				}
				D3D12::TransitionResources(cmdList, transitionBarriers);
				graphicsQueue.ExecuteContext(*cmdContext.m_Context);

				return true;
			}

		private:
			// todo Clean up classes
			struct BufferBinding
			{
				Pipeline::Shader::ShaderResourceInfo BindingInfo;
				std::weak_ptr<D3D12::GPUBuffer> Buffer;
			};

			struct RootConstantBinding
			{
				void* Alloc(int32_t numBytes)
				{
					return std::malloc(numBytes);
				}

				Pipeline::Shader::ShaderResourceInfo BindingInfo;

				// Workaround to enable unique_ptr to delete allocated memory thats referenced as a void*
				std::unique_ptr<void, decltype(&std::free)> Data;
				RootConstantBinding()
					:Data(nullptr, &std::free){ }
				RootConstantBinding(int32_t num32Bit)
					:Data(this->Alloc(num32Bit * sizeof(int32_t)), &std::free)
				{

				}
			};
			//

			struct Bindings
			{
				// todo Replace with RenderSurface?
				std::optional<std::weak_ptr<Rendering::RenderSurface>> DepthStencilTarget;
				std::unordered_map<std::string, std::weak_ptr<Rendering::RenderSurface>> RenderTargets;
				//

				// todo Make less copy-pasta?
				std::unordered_map<std::string, BufferBinding> VSBuffers;
				std::unordered_map<std::string, RootConstantBinding> VSRootConstants;

				std::unordered_map<std::string, BufferBinding> PSBuffers;
				std::unordered_map<std::string, RootConstantBinding> PSRootConstants;
				//

				struct TextureBinding
				{
					std::weak_ptr<D3D12::GPUTexture> Texture;
					D3D12_RESOURCE_STATES InOutState;
					D3D12_RESOURCE_STATES AccessState;
				};
				std::unordered_map<uint32_t, TextureBinding> TextureBindings;
			};

			// todo Make it into less input, maybe through "di"
			bool RenderBatch(ID3D12GraphicsCommandList* cmdList,
				ID3D12DescriptorHeap* resourceHeap,
				ID3D12DescriptorHeap* samplerHeap,
				std::span<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles,
				const Scene::SceneProxy::Camera& camera, 
				uint32_t staticMeshBatchOffset,
				const StaticMeshBatchDrawData& staticMeshBatch, 
				const LightDrawData& lightDrawData)
			{
				ID3D12DescriptorHeap* heaps[2] = { resourceHeap, samplerHeap };
				cmdList->SetDescriptorHeaps(2, heaps);
				cmdList->SetPipelineState(m_PipelineState.Get());
				cmdList->SetGraphicsRootSignature(m_RootSignature.Get());
				cmdList->IASetPrimitiveTopology(this->GetTopologyType1());

				if (m_Bindings.DepthStencilTarget.has_value() && !m_Bindings.DepthStencilTarget->expired())
				{
					if (m_Bindings.DepthStencilTarget.has_value())
					{
						D3D12_CPU_DESCRIPTOR_HANDLE tempHandle = m_Bindings.DepthStencilTarget->lock()->GetView<D3D12::DepthStencilView>().GetDescriptorHandle();
						cmdList->OMSetRenderTargets(rtvHandles.size(), rtvHandles.data(), false, &tempHandle);
					}
					else
					{
						cmdList->OMSetRenderTargets(rtvHandles.size(), rtvHandles.data(), false, nullptr);
					}
				}
				else
				{
					DEBUG_PRINT("Depth stencil expired.");
				}

				// todo For each of these the data is staged into a GPUBuffer that uses a LinearAllocator.
				//			The RenderGraph owns the GPUBuffer and binds the current frame's GPUBuffer before calling ScenePass::Execute().
				//				This is because we need to triple-buffer them.

				// todo Remove the need to bind these via the map for each batch
				//			Maybe D3D12_COMMAND_LIST_TYPE_BUNDLE can be used to avoid the cost of this->BindResources(cmdList)?
				//			VertexPerDrawData, PixelPerDrawData and other mandatory ScenePass bindings should maybe be moved into a hardcoded vector? 
				//				If so, how to initialize that? Regardless, they have to be set every frame since we're using triple-buffering.
				struct VSPerBatchConstants
				{
					uint32_t batchOffset;
					uint32_t meshIndex;
					uint32_t x = 0;
					uint32_t y = 0;
				};
				VSPerBatchConstants vData;
				vData.meshIndex = staticMeshBatch.MeshIndex;
				vData.batchOffset = staticMeshBatchOffset;
				this->BindVertexShaderConstants("PerBatchConstantsBuffer", (void*)&vData);

				struct PSPerBatchConstants
				{
					uint32_t materialIndex;
					uint32_t numDirectionalLights;
					uint32_t numPointLights;
					uint32_t numSpotLights;
				};
				PSPerBatchConstants pData;
				pData.materialIndex = staticMeshBatch.MaterialIndex;
				pData.numPointLights = lightDrawData.NumPointLights;
				pData.numSpotLights = lightDrawData.NumSpotLights;
				pData.numDirectionalLights = lightDrawData.NumDirectionalLights;
				this->BindPixelShaderConstants("PerBatchConstantsBuffer", (void*)&pData);

				if (!this->BindResources(cmdList))
				{
					return false;
				}
				//

				cmdList->RSSetViewports(1, &camera.m_Viewport);
				cmdList->RSSetScissorRects(1, &camera.m_ScizzorRect);

				cmdList->DrawInstanced(staticMeshBatch.NumVertices, staticMeshBatch.InstanceData.size(), 0, 0);

				return true;
			}

			void Reset()
			{
				m_TopologyType = TOPOLOGY_TYPE::INVALID;
				m_VertexShader = std::weak_ptr<Pipeline::VertexShader>();
				m_PixelShader = std::weak_ptr<Pipeline::PixelShader>();
				m_PipelineState = nullptr;
				m_RootSignature = nullptr;
				m_Bindings = Bindings();
			}

			D3D12_PRIMITIVE_TOPOLOGY GetTopologyType1()
			{
				if (m_TopologyType == TOPOLOGY_TYPE::TRIANGLE)
				{
					return D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				}
				else if (m_TopologyType == TOPOLOGY_TYPE::LINE)
				{
					return D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_LINELIST;
				}
				else if (m_TopologyType == TOPOLOGY_TYPE::POINT)
				{
					return D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
				}

				return D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
			}

			D3D12_PRIMITIVE_TOPOLOGY_TYPE GetTopologyType2()
			{
				if (m_TopologyType == TOPOLOGY_TYPE::TRIANGLE)
				{
					return D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
				}
				else if (m_TopologyType == TOPOLOGY_TYPE::LINE)
				{
					return D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
				}
				else if (m_TopologyType == TOPOLOGY_TYPE::POINT)
				{
					return D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
				}

				return D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
			}

			bool GenerateBindings(const PassDescription& description)
			{
				for (auto& [name, bindingInfo] : m_VertexShader.lock()->GetResourceBindings())
				{
					if (bindingInfo.m_ResourceType != D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
					{
						m_Bindings.VSBuffers[name] = BufferBinding();
						auto& buffer = m_Bindings.VSBuffers[name];
						buffer.BindingInfo = bindingInfo;
					}
					else
					{
						m_Bindings.VSRootConstants[name] = RootConstantBinding(bindingInfo.m_Num32BitConstants);
						auto& rc = m_Bindings.VSRootConstants[name];
						rc.BindingInfo = bindingInfo;
					}
				}

				if (!m_PixelShader.expired())
				{
					for (auto& [name, bindingInfo] : m_PixelShader.lock()->GetResourceBindings())
					{
						if (bindingInfo.m_ResourceType != D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
						{
							m_Bindings.PSBuffers[name] = BufferBinding();
							auto& buffer = m_Bindings.PSBuffers[name];
							buffer.BindingInfo = bindingInfo;
						}
						else
						{
							m_Bindings.PSRootConstants[name] = RootConstantBinding(bindingInfo.m_Num32BitConstants);
							auto& rc = m_Bindings.PSRootConstants[name];
							rc.BindingInfo = bindingInfo;
						}
					}
				}

				for (int rtvIndex = 0; rtvIndex < description.RenderTargets.size(); rtvIndex++)
				{
					const PassDescription::RenderTarget& rtv = description.RenderTargets.at(rtvIndex);
					if (rtv.Name.empty())
					{
						DEBUG_PRINT("Render target at index " + std::to_string(rtvIndex) + " didn't have a name");
						return false;
					}
					
					if (m_Bindings.RenderTargets.count(rtv.Name) > 0)
					{
						DEBUG_PRINT("Render target at index " + std::to_string(rtvIndex) + " with name " + rtv.Name + " already exist.");
						return false;
					}

					m_Bindings.RenderTargets[rtv.Name] = std::weak_ptr<Rendering::RenderSurface>();
				}

				m_Bindings.DepthStencilTarget = std::weak_ptr<Rendering::RenderSurface>();

				return true;
			}

			bool ValidateBoundRenderSurfaces()
			{
				//if (m_Bindings.DepthStencilTarget.has_value() && m_Bindings.DepthStencilTarget->Descriptor.expired())
				//{
				//	// LOG
				//	return false;
				//}

				//if (m_Bindings.DepthStencilTarget->Descriptor.lock()->GetHeapIndex() == -1) // todo Change "-1" to a constexpr static var
				//{
				//	// LOG
				//	return false;
				//}

				for (auto& [name, rtv] : m_Bindings.RenderTargets)
				{
					//if (rtv.Descriptor.expired())
					//{
					//	// LOG - Name and reason
					//	return false;
					//}

					//if (rtv.Descriptor.lock()->GetHeapIndex() == -1) // todo Change "-1" to a constexpr static var
					//{
					//	// LOG - Name and reason
					//	return false;
					//}
				}

				return true;
			}

			bool ValidateAndAsignPass(const PassDescription& description, std::weak_ptr<Pipeline::VertexShader> vertexShader, std::optional<std::weak_ptr<Pipeline::PixelShader>> pixelShader)
			{
				if (description.TopologyType == TOPOLOGY_TYPE::INVALID)
				{
					// LOG - Not an acceptable topology
					return false;
				}

				if (description.DepthStencil.Format == DXGI_FORMAT::DXGI_FORMAT_UNKNOWN && description.RenderTargets.size() == 0)
				{
					// LOG - We need the pass to write to atleast an rtv or dsv
					return false;
				}

				if (description.RenderTargets.size() > 8)
				{
					// LOG - Pass doesn't allow more than 8 rtvs
					return false;
				}

				// A graphics pipeline MUST have a vertex shader (if we ignore mesh shaders)
				if (vertexShader.expired())
				{
					// LOG - A vertex shader is always needed so the shader must be valid
					return false;
				}

				if (pixelShader.has_value())
				{
					if (pixelShader->expired())
					{
						// LOG - The pixel shader wasnt valid
						return false;
					}
					m_PixelShader = pixelShader.value();
				}

				m_VertexShader = vertexShader;
				m_TopologyType = description.TopologyType;
				return true;
			}

			bool CreateRootSignature(ID3D12Device* device)
			{
				// todo Dont change the shaders, but instead store local "maps"
				std::vector<D3D12_ROOT_PARAMETER> allParams;
				std::shared_ptr<Pipeline::VertexShader> vertexShader = m_VertexShader.lock();
				for (const D3D12_ROOT_PARAMETER& Param : vertexShader->m_RootParameters)
				{
					allParams.emplace_back(Param);
				}

				if (!m_PixelShader.expired())
				{
					const size_t numVSBinds = vertexShader->m_ResourceNameToInformation.size();
					std::shared_ptr<Pipeline::PixelShader> pixelShader = m_PixelShader.lock();
					for (auto& nameToIndex : pixelShader->m_ResourceNameToInformation)
					{
						nameToIndex.second.m_RootIndex += numVSBinds;
					}

					for (const D3D12_ROOT_PARAMETER& Param : pixelShader->m_RootParameters)
					{
						allParams.emplace_back(Param);
					}
				}

				// todo Fill in?
				std::vector<D3D12_STATIC_SAMPLER_DESC> StaticSamplers;
				//

				const D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc{
					static_cast<UINT>(allParams.size()),
					allParams.data(),
					static_cast<UINT>(StaticSamplers.size()), StaticSamplers.data(),
					D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
					| D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
					| D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED };

				Microsoft::WRL::ComPtr<ID3DBlob> SerializeBlob;
				Microsoft::WRL::ComPtr<ID3DBlob> ErrorBlob;

				const HRESULT RSSerializeRes = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, SerializeBlob.GetAddressOf(), ErrorBlob.GetAddressOf());
				if (FAILED(RSSerializeRes))
				{
					DEBUG_PRINT("Failed to serialize graphics root signature");
					return false;
				}

				const HRESULT RSRes = device->CreateRootSignature(0, SerializeBlob->GetBufferPointer(), SerializeBlob->GetBufferSize(), IID_PPV_ARGS(m_RootSignature.GetAddressOf()));
				if (FAILED(RSRes))
				{
					DEBUG_PRINT("Failed to create graphics root signature");
					return false;
				}

				return true;
			}

			bool CreatePipelineState(ID3D12Device* device, const PassDescription& description)
			{
				D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc;
				ZeroMemory(&pipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
				pipelineStateDesc.pRootSignature = m_RootSignature.Get();
				pipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

				// todo Make this a setting
				DXGI_SAMPLE_DESC sampleDesc{};
				sampleDesc.Count = 1;
				sampleDesc.Quality = 0;
				pipelineStateDesc.SampleDesc = sampleDesc;

				// todo Make this a setting
				D3D12_RASTERIZER_DESC rasterDesc{};
				rasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
				rasterDesc.FrontCounterClockwise = true;
				pipelineStateDesc.RasterizerState = rasterDesc;

				// todo Make this a setting
				D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
				pipelineStateDesc.BlendState = blendDesc;

				std::shared_ptr<Pipeline::VertexShader> vertexShader = m_VertexShader.lock();
				pipelineStateDesc.InputLayout.NumElements = vertexShader->m_InputElementDescs.size();
				pipelineStateDesc.InputLayout.pInputElementDescs = vertexShader->m_InputElementDescs.data();

				pipelineStateDesc.PrimitiveTopologyType = this->GetTopologyType2();

				// todo Make this a setting
				pipelineStateDesc.SampleMask = std::numeric_limits<uint32_t>::max();

				if (!m_PixelShader.expired())
				{
					std::shared_ptr<Pipeline::PixelShader> pixelShader = m_PixelShader.lock();
					pipelineStateDesc.NumRenderTargets = description.RenderTargets.size();

					if (pixelShader->m_RenderTargetMasks.size() != description.RenderTargets.size())
					{
						DEBUG_PRINT("Pixel shaders number of render targets doesn't match the descriptions.");
						return false;
					}

					for (int i = 0; i < description.RenderTargets.size(); i++)
					{
						const UINT RtvNumComponents = D3D12_PROPERTY_LAYOUT_FORMAT_TABLE::GetNumComponentsInFormat(description.RenderTargets.at(i).Format);
						const uint32_t ExpectedNumComponents = pixelShader->m_RenderTargetMasks.at(i);
						if (ExpectedNumComponents == static_cast<uint32_t>(RtvNumComponents))
						{
							pipelineStateDesc.RTVFormats[i] = description.RenderTargets.at(i).Format;
						}
						else
						{
							const LPCSTR FormatName = D3D12_PROPERTY_LAYOUT_FORMAT_TABLE::GetName(description.RenderTargets.at(i).Format);
							DEBUG_PRINT("Expected a DXGI_FORMAT with " << ExpectedNumComponents << " components but " << FormatName << " has " << RtvNumComponents << " components");
							return false;
						}
					}

					D3D12_DEPTH_STENCIL_DESC DepthStencilDesc{};
					DepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
					if (description.DepthStencil.Format == DXGI_FORMAT::DXGI_FORMAT_UNKNOWN)
					{
						DepthStencilDesc.DepthEnable = false;
					}
					else
					{
						pipelineStateDesc.DepthStencilState = DepthStencilDesc;
						pipelineStateDesc.DSVFormat = description.DepthStencil.Format;
					}

					pipelineStateDesc.PS = {
					reinterpret_cast<BYTE*>(pixelShader->m_CompiledShader->GetBufferPointer()),
					pixelShader->m_CompiledShader->GetBufferSize()
					};
				}

				pipelineStateDesc.VS = {
					reinterpret_cast<BYTE*>(vertexShader->m_CompiledShader->GetBufferPointer()),
					vertexShader->m_CompiledShader->GetBufferSize()
				};

				const HRESULT res = device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(m_PipelineState.GetAddressOf()));
				if (FAILED(res))
				{
					DEBUG_PRINT("Failed to create graphics pipelinestate: " + std::to_string(res));
					return false;
				}

				return true;
			}

			bool CompilePassPipeline(ID3D12Device* device, const PassDescription& desc)
			{
				if (!this->CreateRootSignature(device))
				{
					// LOG
					return false;
				}

				if (!this->CreatePipelineState(device, desc))
				{
					// LOG
					return false;
				}

				return true;
			}

			bool BindResources(ID3D12GraphicsCommandList* cmdList)
			{
				// todo Make less copy-pasta
				for (const auto& binding : m_Bindings.VSBuffers)
				{
					if (binding.second.Buffer.expired())
					{
						DEBUG_PRINT("Bound buffer with name " + binding.first + " that was bound to a vertex shader has expired.");
						return false;
					}

					std::shared_ptr<D3D12::GPUBuffer> buffer = binding.second.Buffer.lock();
					const Pipeline::VertexShader::ShaderResourceInfo& bindingInfo = binding.second.BindingInfo;
					switch (bindingInfo.m_ResourceType)
					{
					case D3D12_ROOT_PARAMETER_TYPE_SRV:
					{
						cmdList->SetGraphicsRootShaderResourceView(bindingInfo.m_RootIndex, buffer->GetResource()->GetGPUVirtualAddress());
						break;
					}
					case D3D12_ROOT_PARAMETER_TYPE_UAV:
					{
						cmdList->SetGraphicsRootUnorderedAccessView(bindingInfo.m_RootIndex, buffer->GetResource()->GetGPUVirtualAddress());
						break;
					}
					case D3D12_ROOT_PARAMETER_TYPE_CBV:
					{
						cmdList->SetGraphicsRootConstantBufferView(bindingInfo.m_RootIndex, buffer->GetResource()->GetGPUVirtualAddress());
						break;
					}
					default:
					{
						DEBUG_PRINT("Bound buffer with name " + binding.first + " that was bound to a vertex shader doesn't have a supported root parameter type.");
						return false;
					}
					}
				}

				for (const auto& binding : m_Bindings.VSRootConstants)
				{
					const Pipeline::VertexShader::ShaderResourceInfo& bindingInfo = binding.second.BindingInfo;
					if (bindingInfo.m_ResourceType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
					{
						cmdList->SetGraphicsRoot32BitConstants(bindingInfo.m_RootIndex, bindingInfo.m_Num32BitConstants, binding.second.Data.get(), 0);
					}
					// todo Look if this case even can happen
					else
					{
						DEBUG_PRINT("Bound root constant " + binding.first + " that was bound to a vertex shader is expected to be a root constant, but isn't");
						return false;
					}
				}

				if (!m_PixelShader.expired())
				{
					for (const auto& binding : m_Bindings.PSBuffers)
					{
						if (binding.second.Buffer.expired())
						{
							DEBUG_PRINT("Bound buffer with name " + binding.first + " that was bound to a pixel shader has expired.");
							return false;
						}

						std::shared_ptr<D3D12::GPUBuffer> buffer = binding.second.Buffer.lock();
						const Pipeline::PixelShader::ShaderResourceInfo& bindingInfo = binding.second.BindingInfo;
						switch (bindingInfo.m_ResourceType)
						{
						case D3D12_ROOT_PARAMETER_TYPE_SRV:
						{
							cmdList->SetGraphicsRootShaderResourceView(bindingInfo.m_RootIndex, buffer->GetResource()->GetGPUVirtualAddress());
							break;
						}
						case D3D12_ROOT_PARAMETER_TYPE_UAV:
						{
							cmdList->SetGraphicsRootUnorderedAccessView(bindingInfo.m_RootIndex, buffer->GetResource()->GetGPUVirtualAddress());
							break;
						}
						case D3D12_ROOT_PARAMETER_TYPE_CBV:
						{
							cmdList->SetGraphicsRootConstantBufferView(bindingInfo.m_RootIndex, buffer->GetResource()->GetGPUVirtualAddress());
							break;
						}
						default:
						{
							DEBUG_PRINT("Bound buffer with name " + binding.first + " that was bound to a pixel shader doesn't have a supported root parameter type.");
							return false;
						}
						}
					}

					for (const auto& binding : m_Bindings.PSRootConstants)
					{
						const Pipeline::PixelShader::ShaderResourceInfo& bindingInfo = binding.second.BindingInfo;
						if (bindingInfo.m_ResourceType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
						{
							cmdList->SetGraphicsRoot32BitConstants(bindingInfo.m_RootIndex, bindingInfo.m_Num32BitConstants, binding.second.Data.get(), 0);
						}
						// todo Look if this case even can happen
						else
						{
							DEBUG_PRINT("Bound root constant " + binding.first + " that was bound to a pixel shader is expected to be a root constant, but isn't");
							return false;
						}
					}
				}
				return true;
				//
			}

			std::weak_ptr<Pipeline::VertexShader> m_VertexShader;
			std::weak_ptr<Pipeline::PixelShader> m_PixelShader;
			Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState = nullptr;
			Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
			Bindings m_Bindings;
			TOPOLOGY_TYPE m_TopologyType;
		};
	}
}