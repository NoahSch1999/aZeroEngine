#pragma once
#include "Shader.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class ComputeShader : public Shader
		{
			friend class ShaderPassBase;
			friend class ComputeShaderPass;
		public:
			struct ThreadGroup
			{
				uint32_t x = 0;
				uint32_t y = 0;
				uint32_t z = 0;
			};

			ComputeShader() = default;
			ComputeShader(IDxcCompilerX& compiler, const std::string& path);
			ComputeShader(ComputeShader&& other) noexcept;
			ComputeShader& operator=(ComputeShader&& other) noexcept;

			bool CompileFromFile(IDxcCompilerX& compiler, const std::string& path) override;
			[[nodiscard]] ThreadGroup GetThreadGroups() const { return m_ThreadGroupCount; }

		private:
			static constexpr const char* m_TargetSM = "cs_6_6";
			static constexpr const char* m_ShaderExtension = ".cs.hlsl";

			ThreadGroup m_ThreadGroupCount;

			bool ValidateShaderTypeFromFilepath(const std::string& path) override;

			bool Reflect(Microsoft::WRL::ComPtr<IDxcResult>& compilationResult, Microsoft::WRL::ComPtr<IDxcUtils>& utils);
		};
	}
}