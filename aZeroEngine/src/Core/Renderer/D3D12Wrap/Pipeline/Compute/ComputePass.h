#pragma once
#include "../Shader.h"

namespace aZero
{
	namespace D3D12
	{
		class ComputePass
		{
		public:
			struct PassDescription
			{
				Shader ComputeShader;
				std::vector<D3D12_STATIC_SAMPLER_DESC> StaticSamplers;
			};

		private:
			Microsoft::WRL::ComPtr<ID3D12PipelineState> m_Pso = nullptr;
			Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;

			PassDescription m_Description;
			std::unordered_map<std::string, int> m_ParameterNameToRootSignatureSlot;

			void CreateRootSignatureFromShaders()
			{
				std::vector<D3D12_ROOT_PARAMETER> AllParams;
				for (const auto& [Name, Param] : m_Description.ComputeShader.GetRootConstants())
				{
					D3D12_ROOT_PARAMETER RootParam;
					RootParam.Constants.ShaderRegister = Param.BindingSlot;
					RootParam.Constants.Num32BitValues = Param.Num32BitValues;
					RootParam.Constants.RegisterSpace = 0;
					RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
					RootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // ????
					m_ParameterNameToRootSignatureSlot.emplace(Name, AllParams.size());
					AllParams.push_back(RootParam);
				}

				for (const auto& [Name, Param] : m_Description.ComputeShader.GetRootDescriptors())
				{
					D3D12_ROOT_PARAMETER RootParam;
					RootParam.Descriptor.ShaderRegister = Param.BindingSlot;
					RootParam.Descriptor.RegisterSpace = 0;
					RootParam.ParameterType = Param.ParameterType;
					RootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
					m_ParameterNameToRootSignatureSlot.emplace(Name, AllParams.size());
					AllParams.push_back(RootParam);
				}

				const D3D12_ROOT_SIGNATURE_DESC desc{
					(UINT)AllParams.size(),
					AllParams.data(),
					m_Description.StaticSamplers.size(), m_Description.StaticSamplers.data(),
					D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
					| D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
					| D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED };

				Microsoft::WRL::ComPtr<ID3DBlob> SerializeBlob;
				Microsoft::WRL::ComPtr<ID3DBlob> ErrorBlob;

				if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, SerializeBlob.GetAddressOf(), ErrorBlob.GetAddressOf())))
				{
					// TODO - Add debug error
					throw;
				}

				if (FAILED(gDevice->CreateRootSignature(0, SerializeBlob->GetBufferPointer(), SerializeBlob->GetBufferSize(), IID_PPV_ARGS(m_RootSignature.GetAddressOf()))))
				{
					// TODO - Add debug error
					throw;
				}
			}

			void CreatePipelineStateObject()
			{
				D3D12_COMPUTE_PIPELINE_STATE_DESC ComputeDesc;
				ZeroMemory(&ComputeDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));

				ComputeDesc.pRootSignature = m_RootSignature.Get();
				ComputeDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

				ComputeDesc.CS = {
				reinterpret_cast<BYTE*>(m_Description.ComputeShader.GetShaderBlob()->GetBufferPointer()),
				m_Description.ComputeShader.GetShaderBlob()->GetBufferSize()
				};

				if (FAILED(gDevice->CreateComputePipelineState(&ComputeDesc, IID_PPV_ARGS(m_Pso.GetAddressOf()))))
				{
					throw;
				}
			}

		public:
			ComputePass(const PassDescription& Description)
				:m_Description(Description)
			{
				this->Build();
			}

			~ComputePass()
			{

			}

			ComputePass::PassDescription GetDescripton() const { return m_Description; }

			void Build()
			{
				if (!m_RootSignature.Get())
				{
					CreateRootSignatureFromShaders();
					CreatePipelineStateObject();
				}
			}

			void SetShaderRootConstant(ID3D12GraphicsCommandList* const CmdList, const std::string& ParamName, void* Data)
			{
				CmdList->SetGraphicsRoot32BitConstants(m_ParameterNameToRootSignatureSlot.at(ParamName),
						m_Description.ComputeShader.GetRootConstants().at(ParamName).Num32BitValues,
						Data, 0);
			}

			ComputePass(const ComputePass&) = delete;
			ComputePass(ComputePass&&) = delete;
			ComputePass operator=(const ComputePass&) = delete;
			ComputePass operator=(ComputePass&&) = delete;

			void BeginPass(ID3D12GraphicsCommandList* CmdList)
			{
				CmdList->SetPipelineState(m_Pso.Get());
				CmdList->SetComputeRootSignature(m_RootSignature.Get());
			}

			void Dispatch(ID3D12GraphicsCommandList* CmdList, int X, int Y, int Z)
			{
				CmdList->Dispatch(X, Y, Z);
			}
		};
	}
}