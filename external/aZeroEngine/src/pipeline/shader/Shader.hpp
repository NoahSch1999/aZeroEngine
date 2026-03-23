#pragma once
#include <unordered_map>
#include <string>
#include <fstream>
#include <span>

#include "graphics_api/D3D12Include.hpp"
#include "misc/RelativePathMacros.hpp"
#include "misc/NonCopyable.hpp"

// todo Make all shaders not get changed if compilation fails (similar to how passes are implemented when compiling them)
namespace aZero
{
	namespace Pipeline
	{
		class Shader : public NonCopyable
		{
			friend class ShaderPassBase;
			friend class MultiShaderPass;
			friend class VertexShaderPass;
			friend class MeshShaderPass;
			friend class ComputeShaderPass;
		public:
			struct ShaderResourceInfo
			{
				uint32_t m_RootIndex;
				D3D12_ROOT_PARAMETER_TYPE m_ResourceType;
				uint32_t m_Num32BitConstants = 0;
			};

			Shader() = default;
			Shader(Shader&& other) noexcept;
			Shader& operator=(Shader&& other) noexcept;

			virtual ~Shader() {}

			virtual bool CompileFromFile(IDxcCompilerX& compiler, const std::string& path) = 0;
			const std::unordered_map<std::string, ShaderResourceInfo>& GetResourceBindings() const { return m_ResourceNameToInformation; }
			const std::vector<D3D12_ROOT_PARAMETER>& GetRootParameters() const { return m_RootParameters; }

		private:
			Microsoft::WRL::ComPtr<IDxcBlob> m_CompiledShader;
			std::unordered_map<std::string, ShaderResourceInfo> m_ResourceNameToInformation;
			std::vector<D3D12_ROOT_PARAMETER> m_RootParameters;

		protected:
			bool CompileImpl(IDxcCompilerX& compiler, const std::string& path, const std::string& targetSM, Microsoft::WRL::ComPtr<IDxcResult>& compilationResult, Microsoft::WRL::ComPtr<IDxcUtils>& utils);
			bool ReflectImpl(Microsoft::WRL::ComPtr<IDxcResult>& compilationResult, Microsoft::WRL::ComPtr<IDxcUtils>& utils, D3D12_SHADER_VISIBILITY shaderVisibility, Microsoft::WRL::ComPtr<ID3D12ShaderReflection>& reflection);
			virtual bool ValidateShaderTypeFromFilepath(const std::string& path) = 0;
		};
	}
}