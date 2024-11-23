#pragma once
#include "../Shader.h"
#include "../InputLayout.h"
#include "../../Resources/GPUResource.h"

namespace aZero
{
	namespace D3D12
	{
		// TODO - Create fail-safe so that the user can evaluate if the pass is valid. This way it can be used easier in the editor
		// TODO - Add support for HS, DS, and GS stages
		class GraphicsPass
		{
		public:
			struct DepthStencilDesc
			{
				D3D12_DEPTH_STENCIL_DESC Description;
				DXGI_FORMAT Format = DXGI_FORMAT_FORCE_UINT;
			};

			struct PassDescription
			{
				D3D12_BLEND_DESC BlendDesc;
				D3D12_RASTERIZER_DESC RasterDesc;
				D3D12_INPUT_LAYOUT_DESC InputLayoutDesc;
				D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType;
				DXGI_SAMPLE_DESC SampleDesc;
				DepthStencilDesc DepthStencilDesc;
				std::vector<D3D12_STATIC_SAMPLER_DESC> StaticSamplers;
				Shader VertexShader;
				Shader PixelShader;
			};

		private:
			Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso = nullptr;
			Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;

			PassDescription m_description;

			// TODO - Try to avoid having to go through this hashmap when setting buffers etc
			std::unordered_map<std::string, int> m_vsParameterNameToRootSignatureSlot;
			std::unordered_map<std::string, int> m_psParameterNameToRootSignatureSlot;

			void CreateRootSignatureFromShaders(ID3D12Device* const device)
			{
				std::vector<D3D12_ROOT_PARAMETER> allParams;

				for (const auto& [name, param] : m_description.VertexShader.GetRootConstants())
				{
					D3D12_ROOT_PARAMETER rootParam;
					rootParam.Constants.ShaderRegister = param.BindingSlot;
					rootParam.Constants.Num32BitValues = param.Num32BitValues;
					rootParam.Constants.RegisterSpace = 0;
					rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
					rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
					m_vsParameterNameToRootSignatureSlot.emplace(name, allParams.size());
					allParams.push_back(rootParam);
				}

				for (const auto& [name, param] : m_description.PixelShader.GetRootConstants())
				{
					D3D12_ROOT_PARAMETER rootParam;
					rootParam.Constants.ShaderRegister = param.BindingSlot;
					rootParam.Constants.Num32BitValues = param.Num32BitValues;
					rootParam.Constants.RegisterSpace = 0;
					rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
					rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
					m_psParameterNameToRootSignatureSlot.emplace(name, allParams.size());
					allParams.push_back(rootParam);
				}

				for (const auto& [name, param] : m_description.VertexShader.GetRootDescriptors())
				{
					D3D12_ROOT_PARAMETER rootParam;
					rootParam.Descriptor.ShaderRegister = param.BindingSlot;
					rootParam.Descriptor.RegisterSpace = 0;
					rootParam.ParameterType = param.ParameterType;
					rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
					m_vsParameterNameToRootSignatureSlot.emplace(name, allParams.size());
					allParams.push_back(rootParam);
				}

				for (const auto& [name, param] : m_description.PixelShader.GetRootDescriptors())
				{
					D3D12_ROOT_PARAMETER rootParam;
					rootParam.Descriptor.ShaderRegister = param.BindingSlot;
					rootParam.Descriptor.RegisterSpace = 0;
					rootParam.ParameterType = param.ParameterType;
					rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
					m_psParameterNameToRootSignatureSlot.emplace(name, allParams.size());
					allParams.push_back(rootParam);
				}

				const D3D12_ROOT_SIGNATURE_DESC desc{ 
					(UINT)allParams.size(),
					allParams.data(),
					m_description.StaticSamplers.size(), m_description.StaticSamplers.data(),
					D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
					| D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
					| D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED };

				Microsoft::WRL::ComPtr<ID3DBlob> serializeBlob;
				Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

				if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, serializeBlob.GetAddressOf(), errorBlob.GetAddressOf())))
				{
					// TODO - Add debug error
					throw;
				}

				if (FAILED(device->CreateRootSignature(0, serializeBlob->GetBufferPointer(), serializeBlob->GetBufferSize(), IID_PPV_ARGS(m_rootSignature.GetAddressOf()))))
				{
					// TODO - Add debug error
					throw;
				}
			}

			void CreatePipelineStateObject(ID3D12Device* const device)
			{
				D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
				ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

#ifdef _DEBUG
				// psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG; // TODO - Add support on warp devices
#endif // DEBUG

				psoDesc.pRootSignature = m_rootSignature.Get();
				psoDesc.SampleDesc = m_description.SampleDesc;
				psoDesc.RasterizerState = m_description.RasterDesc;
				psoDesc.BlendState = m_description.BlendDesc;
				psoDesc.InputLayout = m_description.InputLayoutDesc;
				psoDesc.PrimitiveTopologyType = m_description.TopologyType;
				psoDesc.SampleMask = UINT_MAX;

				psoDesc.NumRenderTargets = m_description.PixelShader.GetRenderTargetFormats().size();
				for (int i = 0; i < m_description.PixelShader.GetRenderTargetFormats().size(); i++)
				{
					psoDesc.RTVFormats[i] = m_description.PixelShader.GetRenderTargetFormats()[i];
				}

				if (m_description.DepthStencilDesc.Description.DepthEnable)
				{
					psoDesc.DepthStencilState = m_description.DepthStencilDesc.Description;
					psoDesc.DSVFormat = m_description.DepthStencilDesc.Format;
				}

				psoDesc.VS = {
					reinterpret_cast<BYTE*>(m_description.VertexShader.GetShaderBlob()->GetBufferPointer()),
					m_description.VertexShader.GetShaderBlob()->GetBufferSize()
				};

				psoDesc.PS = {
					reinterpret_cast<BYTE*>(m_description.PixelShader.GetShaderBlob()->GetBufferPointer()),
					m_description.PixelShader.GetShaderBlob()->GetBufferSize()
				};

				HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pso.GetAddressOf()));
				if (FAILED(hr))
				{
					// TODO - Add logging etc...
					throw;
				}
			}

		public:
			GraphicsPass() = default;

			GraphicsPass(ID3D12Device* const device, const PassDescription& description)
				:m_description(description)
			{
				Build(device);
			}

			~GraphicsPass() = default;

			GraphicsPass::PassDescription GetDescripton() const { return m_description; }

			void Build(ID3D12Device* const device)
			{
				if (!m_rootSignature)
				{
					CreateRootSignatureFromShaders(device);
					CreatePipelineStateObject(device);
				}
			}

			void BeginPass(ID3D12GraphicsCommandList* const commandList, ID3D12DescriptorHeap* ResourceHeap, ID3D12DescriptorHeap* SamplerHeap)
			{
				ID3D12DescriptorHeap* heaps[2] = { ResourceHeap, SamplerHeap };
				commandList->SetDescriptorHeaps(2, heaps);

				commandList->SetGraphicsRootSignature(m_rootSignature.Get());
				commandList->SetPipelineState(m_pso.Get());

				switch (m_description.TopologyType)
				{
				case D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE:
				{
					commandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					break;
				}
				case D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE:
				{
					commandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_LINELIST);
					break;
				}
				case D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT:
				{
					commandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
					break;
				}
				default:
					throw;
				}
			}

			template<typename ShaderType>
			void SetShaderRootConstant(ID3D12GraphicsCommandList* const commandList, const std::string& paramName, void* data)
			{
				if constexpr (std::is_same_v<ShaderType, ShaderTypes::VertexShader>)
				{
					commandList->SetGraphicsRoot32BitConstants(m_vsParameterNameToRootSignatureSlot.at(paramName), 
						m_description.VertexShader.GetRootConstants().at(paramName).Num32BitValues,
						data, 0);
				}
				else if constexpr (std::is_same_v<ShaderType, ShaderTypes::PixelShader>)
				{
					commandList->SetGraphicsRoot32BitConstants(m_psParameterNameToRootSignatureSlot.at(paramName),
						m_description.PixelShader.GetRootConstants().at(paramName).Num32BitValues,
						data, 0);
				}
			}

			template<typename ShaderType, typename DescriptorType>
			void SetShaderRootDescriptor(ID3D12GraphicsCommandList* const commandList, const std::string& paramName, D3D12_GPU_VIRTUAL_ADDRESS address)
			{
				if constexpr (std::is_same_v<ShaderType, ShaderTypes::VertexShader>)
				{
					if constexpr (std::is_same_v<DescriptorType, DescriptorTypes::CBV>)
					{
						commandList->SetGraphicsRootConstantBufferView(m_vsParameterNameToRootSignatureSlot.at(paramName), address);
					}
					else if constexpr (std::is_same_v<DescriptorType, DescriptorTypes::SRV>)
					{
						commandList->SetGraphicsRootShaderResourceView(m_vsParameterNameToRootSignatureSlot.at(paramName), address);
					}
					else if constexpr (std::is_same_v<DescriptorType, DescriptorTypes::UAV>)
					{
						commandList->SetGraphicsRootUnorderedAccessView(m_vsParameterNameToRootSignatureSlot.at(paramName), address);
					}
				}
				else if constexpr (std::is_same_v<ShaderType, ShaderTypes::PixelShader>)
				{
					if constexpr (std::is_same_v<DescriptorType, DescriptorTypes::CBV>)
					{
						commandList->SetGraphicsRootConstantBufferView(m_psParameterNameToRootSignatureSlot.at(paramName), address);
					}
					else if constexpr (std::is_same_v<DescriptorType, DescriptorTypes::SRV>)
					{
						commandList->SetGraphicsRootShaderResourceView(m_psParameterNameToRootSignatureSlot.at(paramName), address);
					}
					else if constexpr (std::is_same_v<DescriptorType, DescriptorTypes::UAV>)
					{
						commandList->SetGraphicsRootUnorderedAccessView(m_psParameterNameToRootSignatureSlot.at(paramName), address);
					}
				}
			}

			void SetOutputTargets(ID3D12GraphicsCommandList* const commandList, const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& renderTargets, const D3D12_CPU_DESCRIPTOR_HANDLE* const depthStencil)
			{
				commandList->OMSetRenderTargets(renderTargets.size(), renderTargets.data(), 0, depthStencil);
			}
		
			void SetVertexBuffer(ID3D12GraphicsCommandList* const commandList, const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView, int startSlot, int numViews)
			{
				commandList->IASetVertexBuffers(startSlot, numViews, &vertexBufferView);
			}

			void SetIndexBuffer(ID3D12GraphicsCommandList* const commandList, const D3D12_INDEX_BUFFER_VIEW& indexBufferView, int startSlot, int numViews)
			{
				commandList->IASetIndexBuffer(&indexBufferView);
			}

			void DrawInstanced(ID3D12GraphicsCommandList* const commandList, int vertexCountPerInstance, int instanceCount, int startVertexOffset, int startInstanceOffset)
			{
				commandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexOffset, startInstanceOffset);
			}

			void DrawIndexedInstanced(ID3D12GraphicsCommandList* const commandList, int indexCountPerInstance, int instanceCount, int startVertexOffset, int startIndexOffset, int startInstanceOffset)
			{
				commandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexOffset, startVertexOffset, startInstanceOffset);
			}
		};
	}
}