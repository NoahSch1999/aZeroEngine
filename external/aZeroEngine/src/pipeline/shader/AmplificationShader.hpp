#pragma once
#include "Shader.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class AmplificationShader : public Shader
		{
			friend class ShaderPassBase;
			friend class ScenePass;
			friend class MeshShaderPass;
		public:
			struct ThreadGroup
			{
				uint32_t x = 0;
				uint32_t y = 0;
				uint32_t z = 0;
			};

			AmplificationShader() = default;
			AmplificationShader(IDxcCompilerX& compiler, const std::string& path);
			AmplificationShader(AmplificationShader&& other) noexcept;
			AmplificationShader& operator=(AmplificationShader&& other) noexcept;

			bool CompileFromFile(IDxcCompilerX& compiler, const std::string& path) override;
			[[nodiscard]] ThreadGroup GetThreadGroups() const { return m_ThreadGroupCount; }

		private:
			static constexpr const char* m_TargetSM = "as_6_6";
			static constexpr const char* m_ShaderExtension = ".as.hlsl";

			ThreadGroup m_ThreadGroupCount;

			bool ValidateShaderTypeFromFilepath(const std::string& path) override;
			bool Reflect(Microsoft::WRL::ComPtr<IDxcResult>& compilationResult, Microsoft::WRL::ComPtr<IDxcUtils>& utils);
		};
	}
}