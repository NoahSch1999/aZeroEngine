#pragma once
#include "Shader.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class MeshShader : public Shader
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

			MeshShader() = default;
			MeshShader(IDxcCompilerX& compiler, const std::string& path);
			MeshShader(MeshShader&& other) noexcept;
			MeshShader& operator=(MeshShader&& other) noexcept;

			bool CompileFromFile(IDxcCompilerX& compiler, const std::string& path) override;
			[[nodiscard]] ThreadGroup GetThreadGroups() const { return m_ThreadGroupCount; }

		private:
			static constexpr const char* m_TargetSM = "ms_6_6";
			static constexpr const char* m_ShaderExtension = ".ms.hlsl";
			
			ThreadGroup m_ThreadGroupCount;

			bool ValidateShaderTypeFromFilepath(const std::string& path) override;
			bool Reflect(Microsoft::WRL::ComPtr<IDxcResult>& compilationResult, Microsoft::WRL::ComPtr<IDxcUtils>& utils);

		
		};
	}
}