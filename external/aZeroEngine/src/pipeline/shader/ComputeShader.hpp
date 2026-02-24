#pragma once
#include "Shader.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class ComputeShader : public Shader
		{
			friend class RenderPass;
			friend class ComputeShaderPass;
		public:
			ComputeShader() = default;
			ComputeShader(IDxcCompilerX& compiler, const std::string& path);
			bool CompileFromFile(IDxcCompilerX& compiler, const std::string& path) override;
			[[nodiscard]] ThreadGroup GetThreadGroups() const { return m_ThreadGroupCount; }

		private:
			static constexpr const char* m_TargetSM = "cs_6_6";
			static constexpr const char* m_ShaderExtension = ".cs.hlsl";
			Shader::ThreadGroup m_ThreadGroupCount;

			void Reset();

			bool ValidateShaderTypeFromFilepath(const std::string& path) override;

			bool Reflect(CComPtr<IDxcResult>& compilationResult, CComPtr<IDxcUtils>& utils);
		};
	}
}