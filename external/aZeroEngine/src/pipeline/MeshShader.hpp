#pragma once
#include "Shader.hpp"
#include <array>

namespace aZero
{
	namespace Pipeline
	{
		class MeshShader : public Shader
		{
			friend class RenderPass;
			friend class ScenePass;
		public:
			struct ThreadGroup
			{
				uint32_t x = 0;
				uint32_t y = 0;
				uint32_t z = 0;
			};
		private:
			static constexpr const char* m_TargetSM = "ms_6_6";
			static constexpr const char* m_ShaderExtension = ".ms.hlsl";
			
			ThreadGroup m_ThreadGroupCount;

			void Reset();
			bool ValidateShaderTypeFromFilepath(const std::string& path) override;
			bool Reflect(CComPtr<IDxcResult>& compilationResult, CComPtr<IDxcUtils>& utils);

		public:
			MeshShader() = default;
			MeshShader(IDxcCompiler3& compiler, const std::string& path);
			bool CompileFromFile(IDxcCompiler3& compiler, const std::string& path) override;
			ThreadGroup GetThreadGroups() const { return m_ThreadGroupCount; }
		};
	}
}