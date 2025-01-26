#pragma once
#include <unordered_map>
#include <string>
#include <optional>

#include "Core/D3D12Include.h"
#include "Core/DXCompilerInclude.h"


// TODO: Try to remove this being a dynamic string allocation etc...
// TODO: Add path for compiled shaders
#define SHADER_COMPILED_DIRECTORY std::string("?")
#define SHADER_COMPILED_PATH(ShaderName) (SHADER_COMPILED_DIRECTORY + std::string(ShaderName) + std::string(".?"))

#define SHADER_SRC_DIRECTORY std::string("src/Shaders/")
#define SHADER_SRC_PATH(ShaderName) (SHADER_SRC_DIRECTORY + std::string(ShaderName) + std::string(".hlsl"))

namespace aZero
{
	namespace D3D12
	{
		enum class SHADER_TYPE { NONE, VS, PS, CS };

		class RenderPass;

		class Shader
		{
			friend RenderPass;

		private:
			SHADER_TYPE m_Type;
			CComPtr<IDxcBlob> m_CompiledShader;

			struct ShaderResourceInfo
			{
				uint32_t m_RootIndex;
				D3D12_ROOT_PARAMETER_TYPE m_ResourceType;
				uint32_t m_Num32BitConstants = 0;
			};
			std::unordered_map<std::string, ShaderResourceInfo> m_ResourceNameToInformation;
			std::vector<D3D12_ROOT_PARAMETER> m_RootParameters;

			std::vector<uint32_t> m_RenderTargetMasks;
			std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputElementDescs;
			std::vector<std::string> m_InputElementSemanticNames; // TODO: Try remove

			void Reset()
			{
				m_Type = SHADER_TYPE::NONE;
				m_CompiledShader = nullptr;
				m_ResourceNameToInformation.clear();
				m_RootParameters.clear();
				m_RenderTargetMasks.clear();
				m_InputElementDescs.clear();
				m_InputElementSemanticNames.clear();
			}

			bool ExtractShaderTypeFromFilepath(const std::string& Path, std::string& OutputStr);

			void GenerateReflectionData(IDxcResult& CompiledShaderResult, IDxcUtils& Utils);

		public:

			Shader() = default;

			bool CompileFromFile(IDxcCompiler3& Compiler, const std::string& FilePath);

			SHADER_TYPE GetType() const { return m_Type; }
		};
	}
}