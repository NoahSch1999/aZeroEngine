#pragma once
#include <unordered_map>
#include <string>
#include <optional>

#include "Core/D3D12Include.h"
#include "Core/DXCompilerInclude.h"

namespace aZero
{
	namespace D3D12
	{
		enum class SHADER_TYPE { NONE, VS, PS, CS };

		class RenderPass;

		class Shader
		{
			friend RenderPass;

		public:
			// For use when you need to override a cbuffer or constantbuffer<T> shader resource to be used as a root constant
			struct RootConstantOverride
			{
				uint32_t BindingSlot;
			};

			// For use when you want to specify a specific DXGI_FORMAT for an rendertarget
			// Default values are found in: Shader::ReflectionMaskToDXGIFormat()
			struct RenderTargetOverride
			{
				uint32_t TargetSlot;
				DXGI_FORMAT Format;
			};

		private:
			SHADER_TYPE m_Type;
			CComPtr<IDxcBlob> m_CompiledShader;

			struct ShaderResourceInfo
			{
				uint32_t m_RootIndex;
				D3D12_ROOT_PARAMETER_TYPE m_ResourceType;
			};
			std::unordered_map<std::string, ShaderResourceInfo> m_ResourceNameToInformation;
			std::vector<D3D12_ROOT_PARAMETER> m_RootParameters;

			std::vector<DXGI_FORMAT> m_RenderTargetFormats;
			std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputElementDescs;
			std::vector<std::string> m_InputElementSemanticNames; // TODO: Try remove

			void Reset()
			{
				m_Type = SHADER_TYPE::NONE;
				m_CompiledShader = nullptr;
				m_ResourceNameToInformation.clear();
				m_RootParameters.clear();
				m_RenderTargetFormats.clear();
				m_InputElementDescs.clear();
				m_InputElementSemanticNames.clear();
			}

			bool ExtractShaderTypeFromFilepath(const std::string& Path, std::string& OutputStr);

			void GenerateReflectionData(IDxcResult& CompiledShaderResult, 
				IDxcUtils& Utils, 
				std::optional<std::vector<RootConstantOverride>> RootConstOverride,
				std::optional<std::vector<RenderTargetOverride>> RTOverride);

		public:

			Shader() = default;

			bool CompileFromFile(IDxcCompiler3& Compiler,
				const std::string& FilePath, 
				std::optional<std::vector<RootConstantOverride>> RootConstOverride = std::optional<std::vector<RootConstantOverride>>{},
				std::optional<std::vector<RenderTargetOverride>> RTOverride = std::optional<std::vector<RenderTargetOverride>>{});

			SHADER_TYPE GetType() const { return m_Type; }
		};
	}
}