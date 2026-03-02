#pragma once
#include "Shader.hpp"
#include <array>

namespace aZero
{
	namespace Pipeline
	{
		class MeshShader : public Shader
		{
			friend class ShaderPassBase;
			friend class ScenePass;
			friend class MeshShaderPass;
		private:
			static constexpr const char* m_TargetSM = "ms_6_6";
			static constexpr const char* m_ShaderExtension = ".ms.hlsl";
			
			ThreadGroup m_ThreadGroupCount;

			void Reset();
			bool ValidateShaderTypeFromFilepath(const std::string& path) override;
			bool Reflect(CComPtr<IDxcResult>& compilationResult, CComPtr<IDxcUtils>& utils);

		public:
			MeshShader() = default;
			MeshShader(IDxcCompilerX& compiler, const std::string& path);
			bool CompileFromFile(IDxcCompilerX& compiler, const std::string& path) override;
			[[nodiscard]] ThreadGroup GetThreadGroups() const { return m_ThreadGroupCount; }
		};
	}
}