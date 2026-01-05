#pragma once
#include "RenderPass_New.hpp"
#include "graphics_api/CommandQueue.hpp"
#include "ComputeShader.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class ComputePass : public RenderPass_New
		{
		public:
			struct DispatchCount
			{
				uint32_t x = 0;
				uint32_t y = 0;
				uint32_t z = 0;
			};

			struct PassDescription
			{
				DispatchCount DispatchCount;
			};

			ComputePass() = default;
			bool Execute(D3D12::CommandQueue& graphicsQueue,
				D3D12::CommandContextAllocator::CommandContextHandle& cmdContext,
				ID3D12DescriptorHeap* resourceHeap,
				ID3D12DescriptorHeap* samplerHeap)
			{

			}

			void Compile(ID3D12DeviceX* device, const PassDescription& description, std::weak_ptr<Pipeline::ComputeShader> computeShader)
			{
				if (!this->ValidateAndAsignPass(description, computeShader))
				{
					this->Reset();
					throw std::runtime_error("Validation of ComputePas failed.");
				}

				if (!this->CompilePassPipeline(device, description))
				{
					this->Reset();
					throw std::runtime_error("Failed to compile ComputePas.");
				}

				if (!this->GenerateBindings(description))
				{
					this->Reset();
					throw std::runtime_error("Failed to generate bindings for ComputePas.");
				}
			}

			void SetDispatchCount(DispatchCount&& dispatchCount)
			{
				m_DispatchCount = std::move(dispatchCount);
			}

		private:
			void Reset()
			{
				m_DispatchCount = DispatchCount();
				m_Bindings = Bindings();
				m_ComputeShader = std::weak_ptr<Pipeline::ComputeShader>();
				m_PipelineState = nullptr;
				m_RootSignature = nullptr;
			}

			bool ValidateAndAsignPass(const PassDescription& description, std::weak_ptr<Pipeline::ComputeShader> computeShader)
			{
				if (computeShader.expired())
				{
					return false;
				}

				// NOTE: No DispatchCount check here cause someone might wanna dispatch 0,0,0 for some reason

				m_ComputeShader = computeShader;
				m_DispatchCount = description.DispatchCount;
				return true;
			}

			bool CompilePassPipeline(ID3D12DeviceX* device, const PassDescription& desc)
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

			bool CreateRootSignature(ID3D12DeviceX* device)
			{
				std::vector<D3D12_ROOT_PARAMETER> allParams;
				std::shared_ptr<Pipeline::ComputeShader> computeShader = m_ComputeShader.lock();
				for (const D3D12_ROOT_PARAMETER& Param : computeShader->m_RootParameters)
				{
					allParams.emplace_back(Param);
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
					DEBUG_PRINT("Failed to serialize compute root signature");
					return false;
				}

				const HRESULT RSRes = device->CreateRootSignature(0, SerializeBlob->GetBufferPointer(), SerializeBlob->GetBufferSize(), IID_PPV_ARGS(m_RootSignature.GetAddressOf()));
				if (FAILED(RSRes))
				{
					DEBUG_PRINT("Failed to create compute root signature");
					return false;
				}

				return true;
			}

			bool CreatePipelineState(ID3D12DeviceX* device, const PassDescription& description)
			{
				D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineStateDesc;
				pipelineStateDesc.pRootSignature = m_RootSignature.Get();
				pipelineStateDesc.NodeMask = 0; // Single GPU
				pipelineStateDesc.CachedPSO.pCachedBlob = nullptr;
				pipelineStateDesc.CachedPSO.CachedBlobSizeInBytes = 0;

#if USE_DEBUG
				pipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
#else
				pipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
#endif // DEBUG
				std::shared_ptr<Pipeline::ComputeShader> computeShader = computeShader.lock();
				pipelineStateDesc.CS = {
					reinterpret_cast<BYTE*>(computeShader->m_CompiledShader->GetBufferPointer()),
					computeShader->m_CompiledShader->GetBufferSize()
				};

				const HRESULT res = device->CreateComputePipelineState(&pipelineStateDesc, IID_PPV_ARGS(m_PipelineState.GetAddressOf()));
				if (FAILED(res))
				{
					DEBUG_PRINT("Failed to create compute pipelinestate: " + std::to_string(res));
					return false;
				}

				return true;
			}

			bool GenerateBindings(const PassDescription& description)
			{
				for (auto& [name, bindingInfo] : m_ComputeShader.lock()->GetResourceBindings())
				{
					if (bindingInfo.m_ResourceType != D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
					{
						m_Bindings.Buffers[name] = BufferBinding();
						auto& buffer = m_Bindings.Buffers[name];
						buffer.BindingInfo = bindingInfo;
					}
					else
					{
						m_Bindings.RootConstants[name] = RootConstantBinding(bindingInfo.m_Num32BitConstants);
						auto& rc = m_Bindings.RootConstants[name];
						rc.BindingInfo = bindingInfo;
					}
				}

				return true;
			}

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
					:Data(nullptr, &std::free) {
				}
				RootConstantBinding(int32_t num32Bit)
					:Data(this->Alloc(num32Bit * sizeof(int32_t)), &std::free)
				{

				}
			};

			struct Bindings
			{
				std::unordered_map<std::string, BufferBinding> Buffers;
				std::unordered_map<std::string, RootConstantBinding> RootConstants;
			};

			std::weak_ptr<Pipeline::ComputeShader> m_ComputeShader;
			Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
			Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
			Bindings m_Bindings;
			DispatchCount m_DispatchCount;
		};
	}
}