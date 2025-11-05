#pragma once
#include "Shader.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class ComputeShader : public Shader
		{
			friend class RenderPass;
		private:
			static constexpr const char* m_TargetSM = "cs_6_6";
			static constexpr const char* m_ShaderExtension = ".cs.hlsl";
			uint32_t m_ThreadsX;
			uint32_t m_ThreadsY;
			uint32_t m_ThreadsZ;

			void Reset();

			bool ValidateShaderTypeFromFilepath(const std::string& path) override;

			bool Reflect(CComPtr<IDxcResult>& compilationResult, CComPtr<IDxcUtils>& utils);

		public:
			ComputeShader() = default;
			ComputeShader(IDxcCompiler3& compiler, const std::string& path);
			bool CompileFromFile(IDxcCompiler3& compiler, const std::string& path) override;
		};
	}
}