#include "render_graph/Shader.hpp"
#include "misc/NonCopyable.hpp"
#include <optional>

// TODO: Replace rendergraph etc...
namespace aZero
{
	namespace Rendering
	{
		class RenderPassNew : public NonCopyable
		{
		public:
			Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
			Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;

			D3D12_PRIMITIVE_TOPOLOGY_TYPE m_TopologyType;
			uint32_t m_NumRenderTargets;
			bool m_HasDepthStencilTarget;

			std::unordered_map<std::string, D3D12::Shader::ShaderResourceInfo> m_VertexShaderBindings;
			std::unordered_map<std::string, D3D12::Shader::ShaderResourceInfo> m_PixelShaderBindings;

			void Init(
				ID3D12Device* device, 
				const D3D12::Shader& vertexShader, 
				const D3D12::Shader& pixelShader, 
				const std::vector<DXGI_FORMAT>& renderTargetFormats, 
				std::optional<DXGI_FORMAT> depthStencilFormat, 
				D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
			{
				// TODO: Do input validation

				// TODO: Handle re-init

				m_TopologyType = TopologyType;
				m_NumRenderTargets = PixelShader.NumRenderTargets();
				m_HasDepthStencilTarget = depthStencilFormat.has_value();

				// Create root signature 
			}

		private:
		};
	}
}