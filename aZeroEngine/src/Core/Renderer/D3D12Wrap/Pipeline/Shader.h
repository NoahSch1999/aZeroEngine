#pragma once
#include <unordered_map>
#include <string>

#include "Core/D3D12Core.h"

namespace aZero
{
	namespace D3D12
	{
		Microsoft::WRL::ComPtr<ID3DBlob> LoadBlobFromFile(const std::string& FilePath);

		struct ShaderTypes
		{
			class VertexShader
			{
				bool dummy;
			};

			class PixelShader
			{
				bool dummy;
			};
		};

		struct DescriptorTypes
		{
			class CBV
			{
				bool dummy;
			};

			class SRV
			{
				bool dummy;
			};

			class UAV
			{
				bool dummy;
			};
		};

		class Shader
		{
		public:
			struct ShaderParameter
			{
				std::string Name = "INVALID";
				int BindingSlot = -1;
				ShaderParameter(const std::string& name, int bindingSlot)
				{
					Name = name;
					BindingSlot = bindingSlot;
				}
			};

			struct RootConstant : public ShaderParameter
			{
				int Num32BitValues = 0;
				RootConstant(const std::string& name, int bindingSlot, int num32BitValues)
					:ShaderParameter(name, bindingSlot)
				{
					Num32BitValues = num32BitValues;
				}
			};

			struct RootDescriptor : public ShaderParameter
			{
				D3D12_ROOT_PARAMETER_TYPE ParameterType;
				RootDescriptor(const std::string& name, int bindingSlot, D3D12_ROOT_PARAMETER_TYPE parameterType)
					:ShaderParameter(name, bindingSlot)
				{
					ParameterType = parameterType;
				}
			};

			struct StaticSamplerDesc
			{
				D3D12_STATIC_SAMPLER_DESC Desc;
			};

		private:

			std::unordered_map<std::string, RootConstant> m_rootConstants;
			std::unordered_map<std::string, RootDescriptor> m_rootDescriptor;

			// TODO - samplers and rtv formats should be deduced from the shader compiler and passed to the pipeline pass creation
			std::vector<StaticSamplerDesc> m_staticSamplers; // NOTE - Only for none-COMPUTE type shaders
			std::vector<DXGI_FORMAT> m_renderTargets; // NOTE - Only for PS type shaders

			Microsoft::WRL::ComPtr<ID3DBlob> m_compiledShader = nullptr;

			void CompileFromFile(const std::string& FilePath)
			{
				// TODO - Impl

				const std::wstring FilePathWStr(FilePath.begin(), FilePath.end());


				// Fill param maps with data
			}

		public:

			Shader() = default;

			// TODO - Remove type input and replace with loaded shaders type automatically
			Shader(const std::string& srcFilePath)
			{
				Initialize(srcFilePath);
			}

			void Initialize(const std::string& srcFilePath);

			ID3DBlob* const GetShaderBlob() const { return m_compiledShader.Get(); }

			// TODO - The goal is that all adding should be done automatically through the CompileFromFile method.
			// Thus, these methods should be removed when that is implemented.
			template<typename Parameter>
			void AddParameter(Parameter&& param)
			{
				if constexpr (std::is_same_v<Parameter, RootConstant>)
				{
					m_rootConstants.emplace(param.Name, param);
				}
				else if constexpr (std::is_same_v<Parameter, RootDescriptor>)
				{
					m_rootDescriptor.emplace(param.Name, param);
				}
				else
				{
					// TODO - Add fatal error debug
					throw;
				}
			}

			void AddRenderTarget(DXGI_FORMAT format)
			{
				m_renderTargets.push_back(format);
			}

			void AddStaticSampler(const StaticSamplerDesc& sampler)
			{
				m_staticSamplers.push_back(sampler);
			}

			const std::unordered_map<std::string, RootConstant>& GetRootConstants() const
			{
				return m_rootConstants;
			}

			const std::unordered_map<std::string, RootDescriptor>& GetRootDescriptors() const
			{
				return m_rootDescriptor;
			}

			const std::vector<DXGI_FORMAT>& GetRenderTargetFormats() const
			{
				return m_renderTargets;
			}
		};
	}
}