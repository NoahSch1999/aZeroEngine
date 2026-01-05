#pragma once
#include <unordered_map>
#include <string>
#include <fstream>
#include <span>

#include "graphics_api/D3D12Include.hpp"
#include "misc/RelativePathMacros.hpp"

namespace aZero
{
	namespace Pipeline
	{
		enum class SHADER_TYPE { NONE, VS, PS, CS }; // todo Legacy shaders - Remove
		class Shader
		{
			friend class RenderPass;
			friend class ScenePass;

			friend class NewRenderPass;
			friend class MultiShaderPass;
			friend class VertexShaderPass;
			friend class MeshShaderPass;
			friend class ComputeShaderPass;
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

			bool CompileImpl(IDxcCompilerX& compiler, const std::string& path, const std::string& targetSM, CComPtr<IDxcResult>& compilationResult, CComPtr<IDxcUtils>& utils);

			bool ReflectImpl(CComPtr<IDxcResult>& compilationResult, CComPtr<IDxcUtils>& utils, D3D12_SHADER_VISIBILITY shaderVisibility, Microsoft::WRL::ComPtr<ID3D12ShaderReflection>& reflection);

			virtual bool ValidateShaderTypeFromFilepath(const std::string& path) = 0;
		public:

			Shader() = default;
			virtual ~Shader() {}

			virtual bool CompileFromFile(IDxcCompilerX& compiler, const std::string& path) = 0;
			const std::unordered_map<std::string, ShaderResourceInfo>& GetResourceBindings() const { return m_ResourceNameToInformation; }
			const std::vector<D3D12_ROOT_PARAMETER>& GetRootParameters() const { return m_RootParameters; }

		public:
			struct ThreadGroup
			{
				uint32_t x = 0;
				uint32_t y = 0;
				uint32_t z = 0;
			};
		};
	}
}