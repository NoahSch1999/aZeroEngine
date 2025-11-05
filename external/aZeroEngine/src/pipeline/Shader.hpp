#pragma once
#include <unordered_map>
#include <string>
#include <fstream>

#include "graphics_api/D3D12Include.hpp"
#include "graphics_api/DXCompilerInclude.hpp"
#include "misc/RelativePathMacros.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class ScenePass;
		class RenderPass;
		enum class SHADER_TYPE { NONE, VS, PS, CS }; // TODO: Legacy shaders - Remove
		class Shader
		{
			friend Pipeline::RenderPass;
			friend Pipeline::ScenePass;
		private:
			CComPtr<IDxcBlob> m_CompiledShader;

			struct ShaderResourceInfo
			{
				uint32_t m_RootIndex;
				D3D12_ROOT_PARAMETER_TYPE m_ResourceType;
				uint32_t m_Num32BitConstants = 0;
			};
			std::unordered_map<std::string, ShaderResourceInfo> m_ResourceNameToInformation;
			std::vector<D3D12_ROOT_PARAMETER> m_RootParameters;

		protected:
			void Reset();

			bool CompileImpl(IDxcCompiler3& compiler, const std::string& path, const std::string& targetSM, CComPtr<IDxcResult>& compilationResult, CComPtr<IDxcUtils>& utils);

			bool ReflectImpl(CComPtr<IDxcResult>& compilationResult, CComPtr<IDxcUtils>& utils, D3D12_SHADER_VISIBILITY shaderVisibility, Microsoft::WRL::ComPtr<ID3D12ShaderReflection>& reflection);

			virtual bool ValidateShaderTypeFromFilepath(const std::string& path) = 0;
		public:

			Shader() = default;
			virtual ~Shader() {}

			virtual bool CompileFromFile(IDxcCompiler3& compiler, const std::string& path) = 0;
			const std::unordered_map<std::string, ShaderResourceInfo>& GetResourceBindings() const { return m_ResourceNameToInformation; }

		};
	}
}