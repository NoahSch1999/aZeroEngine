#pragma once
#include "Shader.h"

namespace aZero
{
	namespace D3D12
	{
		class RenderPass
		{
		private:
			void Reset()
			{
				m_PipelineState = nullptr;
				m_RootSignature = nullptr;
				m_VSResourceMap.clear();
				m_PSResourceMap.clear();
				m_CSResourceMap.clear();
				m_TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
			}

			Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
			Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
			
			std::unordered_map<std::string, Shader::ShaderResourceInfo> m_VSResourceMap;
			std::unordered_map<std::string, Shader::ShaderResourceInfo> m_PSResourceMap;
			std::unordered_map<std::string, Shader::ShaderResourceInfo> m_CSResourceMap;

			D3D12_PRIMITIVE_TOPOLOGY_TYPE m_TopologyType;

		public:

			RenderPass() = default;

			bool Init(ID3D12Device* Device, const Shader& VertexShader, const Shader& PixelShader, DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_UNKNOWN, D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
			{
				if (VertexShader.GetType() != SHADER_TYPE::VS && VertexShader.GetType() != SHADER_TYPE::PS)
				{
					DEBUG_PRINT("RenderPass::Init() => One or more input shader didn't form a valid graphics pipeline");
					return false;
				}

				this->Reset();

				m_TopologyType = TopologyType;
				m_VSResourceMap = VertexShader.m_ResourceNameToInformation;
				m_PSResourceMap = PixelShader.m_ResourceNameToInformation;

				const size_t NumVSBinds = VertexShader.m_ResourceNameToInformation.size();
				for (auto& NameToIndex : m_PSResourceMap)
				{
					NameToIndex.second.m_RootIndex = NameToIndex.second.m_RootIndex + NumVSBinds;
				}

				std::vector<D3D12_ROOT_PARAMETER> AllParams;
				AllParams.reserve(VertexShader.m_RootParameters.size() + PixelShader.m_RootParameters.size());
				for (const D3D12_ROOT_PARAMETER& Param : VertexShader.m_RootParameters)
				{
					AllParams.emplace_back(Param);
				}

				for (const D3D12_ROOT_PARAMETER& Param : PixelShader.m_RootParameters)
				{
					AllParams.emplace_back(Param);
				}

				std::vector<D3D12_STATIC_SAMPLER_DESC> StaticSamplers;

				const D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc{
					(UINT)AllParams.size(),
					AllParams.data(),
					StaticSamplers.size(), StaticSamplers.data(),
					D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
					| D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
					| D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED };

				Microsoft::WRL::ComPtr<ID3DBlob> SerializeBlob;
				Microsoft::WRL::ComPtr<ID3DBlob> ErrorBlob;

				const HRESULT RSSerializeRes = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, SerializeBlob.GetAddressOf(), ErrorBlob.GetAddressOf());
				if (FAILED(RSSerializeRes))
				{
					DEBUG_PRINT("RenderPass::Init() => Failed to serialize graphics root signature");
					return false;
				}

				const HRESULT RSRes = Device->CreateRootSignature(0, SerializeBlob->GetBufferPointer(), SerializeBlob->GetBufferSize(), IID_PPV_ARGS(m_RootSignature.GetAddressOf()));
				if (FAILED(RSRes))
				{
					DEBUG_PRINT("RenderPass::Init() => Failed to create graphics root signature");
					return false;
				}

				// Create pso
				D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineStateDesc;
				ZeroMemory(&PipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
				PipelineStateDesc.pRootSignature = m_RootSignature.Get();
				PipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

				DXGI_SAMPLE_DESC SampleDesc{};
				SampleDesc.Count = 1;
				SampleDesc.Quality = 0;
				PipelineStateDesc.SampleDesc = SampleDesc;

				D3D12_RASTERIZER_DESC RasterDesc{};
				RasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
				PipelineStateDesc.RasterizerState = RasterDesc;

				D3D12_BLEND_DESC BlendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
				PipelineStateDesc.BlendState = BlendDesc;

				PipelineStateDesc.InputLayout.NumElements = VertexShader.m_InputElementDescs.size();
				PipelineStateDesc.InputLayout.pInputElementDescs = VertexShader.m_InputElementDescs.data();

				PipelineStateDesc.PrimitiveTopologyType = TopologyType;
				PipelineStateDesc.SampleMask = UINT_MAX;

				PipelineStateDesc.NumRenderTargets = PixelShader.m_RenderTargetFormats.size();
				for (int i = 0; i < PixelShader.m_RenderTargetFormats.size(); i++)
				{
					PipelineStateDesc.RTVFormats[i] = PixelShader.m_RenderTargetFormats[i];
				}

				D3D12_DEPTH_STENCIL_DESC DepthStencilDesc{};
				DepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
				if (DepthStencilDesc.DepthEnable)
				{
					PipelineStateDesc.DepthStencilState = DepthStencilDesc;
					PipelineStateDesc.DSVFormat = DepthStencilFormat;
				}

				PipelineStateDesc.VS = {
					reinterpret_cast<BYTE*>(VertexShader.m_CompiledShader->GetBufferPointer()),
					VertexShader.m_CompiledShader->GetBufferSize()
				};

				PipelineStateDesc.PS = {
					reinterpret_cast<BYTE*>(PixelShader.m_CompiledShader->GetBufferPointer()),
					PixelShader.m_CompiledShader->GetBufferSize()
				};

				const HRESULT PSORes = Device->CreateGraphicsPipelineState(&PipelineStateDesc, IID_PPV_ARGS(m_PipelineState.GetAddressOf()));
				if (FAILED(PSORes))
				{
					DEBUG_PRINT("RenderPass::Init() => Failed to create graphics pipelinestate");
					return false;
				}

				return true;
			}
		
			bool Init(ID3D12Device* Device, const D3D12::Shader& ComputeShader)
			{
				if (ComputeShader.GetType() != SHADER_TYPE::CS)
				{
					DEBUG_PRINT("RenderPass::Init() => Input shader didn't form a valid compute pipeline");
					return false;
				}

				this->Reset();

				m_CSResourceMap = ComputeShader.m_ResourceNameToInformation;

				std::vector<D3D12_STATIC_SAMPLER_DESC> StaticSamplers;

				const D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc{
					(UINT)ComputeShader.m_RootParameters.size(),
					ComputeShader.m_RootParameters.data(),
					StaticSamplers.size(), StaticSamplers.data(),
					D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
					| D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
					| D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED };

				Microsoft::WRL::ComPtr<ID3DBlob> SerializeBlob;
				Microsoft::WRL::ComPtr<ID3DBlob> ErrorBlob;

				const HRESULT RSSerializeRes = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, SerializeBlob.GetAddressOf(), ErrorBlob.GetAddressOf());
				if (FAILED(RSSerializeRes))
				{
					DEBUG_PRINT("RenderPass::Init() => Failed to serialize compute root signature");
					return false;
				}

				const HRESULT RSRes = Device->CreateRootSignature(0, SerializeBlob->GetBufferPointer(), SerializeBlob->GetBufferSize(), IID_PPV_ARGS(m_RootSignature.GetAddressOf()));
				if (FAILED(RSRes))
				{
					DEBUG_PRINT("RenderPass::Init() => Failed to create compute root signature");
					return false;
				}

				D3D12_COMPUTE_PIPELINE_STATE_DESC PipelineDesc{};
				PipelineDesc.pRootSignature = m_RootSignature.Get();
				PipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

				PipelineDesc.CS = {
					reinterpret_cast<BYTE*>(ComputeShader.m_CompiledShader->GetBufferPointer()),
					ComputeShader.m_CompiledShader->GetBufferSize()
				};

				PipelineDesc.NodeMask = 0;
				PipelineDesc.CachedPSO.CachedBlobSizeInBytes = 0;
				PipelineDesc.CachedPSO.pCachedBlob = NULL;

				const HRESULT PSORes = Device->CreateComputePipelineState(&PipelineDesc, IID_PPV_ARGS(m_PipelineState.GetAddressOf()));
				if (FAILED(PSORes))
				{
					DEBUG_PRINT("RenderPass::Init() => Failed to create compute pipelinestate");
					return false;
				}

				return true;
			}

			ID3D12PipelineState* GetPipelineState() { return m_PipelineState.Get(); }
			ID3D12RootSignature* GetRootSignature() { return m_RootSignature.Get(); }
			bool IsComputePass() const { return m_TopologyType == D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED; }
			D3D12_PRIMITIVE_TOPOLOGY_TYPE GetTopologyType() const { return m_TopologyType; }

			void SetShaderResource(ID3D12GraphicsCommandList* CmdList, const std::string& ShaderResourceName, void* Data, uint32_t SizeBytes, SHADER_TYPE Type)
			{
				Shader::ShaderResourceInfo* Info;
				if (Type == SHADER_TYPE::VS)
				{
					if (m_VSResourceMap.count(ShaderResourceName))
					{
						Info = &m_VSResourceMap.at(ShaderResourceName);
					}
					else
					{
						DEBUG_PRINT("RenderPass::SetShaderResource() => Vertex shader has no parameter of name: " + ShaderResourceName);
						return;
					}
				}
				else if (Type == SHADER_TYPE::PS)
				{
					if (m_PSResourceMap.count(ShaderResourceName))
					{
						Info = &m_PSResourceMap.at(ShaderResourceName);
					}
					else
					{
						DEBUG_PRINT("RenderPass::SetShaderResource() => Pixel shader has no parameter of name: " + ShaderResourceName);
						return;
					}
				}
				else if (Type == SHADER_TYPE::CS)
				{
					if (m_CSResourceMap.count(ShaderResourceName))
					{
						Info = &m_CSResourceMap.at(ShaderResourceName);
					}
					else
					{
						DEBUG_PRINT("RenderPass::SetShaderResource() => Compute shader has no parameter of name: " + ShaderResourceName);
						return;
					}
				}
				else
				{
					DEBUG_PRINT("RenderPass::SetShaderResource() => Input shader type isn't supported");
					return;
				}

				if (Type == SHADER_TYPE::CS)
				{
					if (Info->m_ResourceType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
					{
						CmdList->SetComputeRoot32BitConstants(Info->m_RootIndex, SizeBytes / sizeof(uint32_t), Data, 0);
					}
					else
					{
						DEBUG_PRINT("RenderPass::SetShaderResource() => Input parameter '" + ShaderResourceName + "' is expected to be a root constant, but isn't");
					}
				}
				else if(Type != SHADER_TYPE::NONE)
				{
					if (Info->m_ResourceType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
					{
						CmdList->SetGraphicsRoot32BitConstants(Info->m_RootIndex, SizeBytes / sizeof(uint32_t), Data, 0);
					}
					else
					{
						DEBUG_PRINT("RenderPass::SetShaderResource() => Input parameter '" + ShaderResourceName + "' is expected to be a root constant, but isn't");
					}
				}
				else
				{
					DEBUG_PRINT("RenderPass::SetShaderResource() => No resource set because input type is SHADER_TYPE::NONE");
				}
				
			}

			void SetShaderResource(ID3D12GraphicsCommandList* CmdList, const std::string& ShaderResourceName, D3D12_GPU_VIRTUAL_ADDRESS VirtualAddress, SHADER_TYPE Type)
			{
				Shader::ShaderResourceInfo* Info;
				if (Type == SHADER_TYPE::VS)
				{
					if (m_VSResourceMap.count(ShaderResourceName))
					{
						Info = &m_VSResourceMap.at(ShaderResourceName);
					}
					else
					{
						DEBUG_PRINT("RenderPass::SetShaderResource() => Vertex shader has no parameter of name: " + ShaderResourceName);
						return;
					}
				}
				else if (Type == SHADER_TYPE::PS)
				{
					if (m_PSResourceMap.count(ShaderResourceName))
					{
						Info = &m_PSResourceMap.at(ShaderResourceName);
					}
					else
					{
						DEBUG_PRINT("RenderPass::SetShaderResource() => Pixel shader has no parameter of name: " + ShaderResourceName);
						return;
					}
				}
				else if (Type == SHADER_TYPE::CS)
				{
					if (m_CSResourceMap.count(ShaderResourceName))
					{
						Info = &m_CSResourceMap.at(ShaderResourceName);
					}
					else
					{
						DEBUG_PRINT("RenderPass::SetShaderResource() => Compute shader has no parameter of name: " + ShaderResourceName);
						return;
					}
				}
				else
				{
					DEBUG_PRINT("RenderPass::SetShaderResource() => Input shader type isn't supported");
					return;
				}

				if (Type == SHADER_TYPE::CS)
				{
					switch (Info->m_ResourceType)
					{
						case D3D12_ROOT_PARAMETER_TYPE_SRV:
						{
							CmdList->SetComputeRootShaderResourceView(Info->m_RootIndex, VirtualAddress);
							break;
						}
						case D3D12_ROOT_PARAMETER_TYPE_UAV:
						{
							CmdList->SetComputeRootUnorderedAccessView(Info->m_RootIndex, VirtualAddress);
							break;
						}
						case D3D12_ROOT_PARAMETER_TYPE_CBV:
						{
							CmdList->SetComputeRootConstantBufferView(Info->m_RootIndex, VirtualAddress);
							break;
						}
						default:
						{
							DEBUG_PRINT("RenderPass::SetShaderResource() => Parameter doesn't have a supported parameter type or potentially a root constant");
						}
					}
				}
				else if(Type != SHADER_TYPE::NONE)
				{
					switch (Info->m_ResourceType)
					{
						case D3D12_ROOT_PARAMETER_TYPE_SRV:
						{
							CmdList->SetGraphicsRootShaderResourceView(Info->m_RootIndex, VirtualAddress);
							break;
						}
						case D3D12_ROOT_PARAMETER_TYPE_UAV:
						{
							CmdList->SetGraphicsRootUnorderedAccessView(Info->m_RootIndex, VirtualAddress);
							break;
						}
						case D3D12_ROOT_PARAMETER_TYPE_CBV:
						{
							CmdList->SetGraphicsRootConstantBufferView(Info->m_RootIndex, VirtualAddress);
							break;
						}
						default:
						{
							DEBUG_PRINT("RenderPass::SetShaderResource() => Parameter doesn't have a supported parameter type");
						}
					}
				}
				else
				{
					DEBUG_PRINT("RenderPass::SetShaderResource() => No resource set because input type is SHADER_TYPE::NONE");
				}
			}
		};
	}
}