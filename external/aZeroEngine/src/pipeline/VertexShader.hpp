#pragma once
#include "Shader.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class VertexShader : public Shader
		{
			friend class RenderPass;
			friend class ScenePass;
			friend class NewRenderPass;
			friend class VertexShaderPass;
		private:
			std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputElementDescs;
			std::vector<std::string> m_InputElementSemanticNames; // todo Try remove
			static constexpr const char* m_TargetSM = "vs_6_6";
			static constexpr const char* m_ShaderExtension = ".vs.hlsl";

			void Reset();
			bool ValidateShaderTypeFromFilepath(const std::string& path) override;
			DXGI_FORMAT ReflectionMaskToDXGIFormat(BYTE Mask);
			bool Reflect(CComPtr<IDxcResult>& compilationResult, CComPtr<IDxcUtils>& utils);

		public:
			VertexShader() = default;
			VertexShader(IDxcCompiler3& compiler, const std::string& path);
			bool CompileFromFile(IDxcCompiler3& compiler, const std::string& path) override;
		};
	}
}