#pragma once
#include "Shader.hpp"
#include <span>
#include <array>

namespace aZero
{
	namespace Pipeline
	{
		class PixelShader final : public Shader
		{
			friend class RenderPass;
			friend class ScenePass;
			friend class NewRenderPass;
			friend class VertexShaderPass;
		private:
			enum NUM_RTV_CHANNELS { R = 1, RG = 2, RGB = 3, RGBA = 4 };

			static constexpr std::array<DXGI_FORMAT, 3> m_ValidFormatsRGBA = { DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_SNORM };
			static constexpr std::array<DXGI_FORMAT, 0> m_ValidFormatsRGB = { };
			static constexpr std::array<DXGI_FORMAT, 2> m_ValidFormatsRG = { };
			static constexpr std::array<DXGI_FORMAT, 3> m_ValidFormatsR = { DXGI_FORMAT::DXGI_FORMAT_R32_UINT, DXGI_FORMAT::DXGI_FORMAT_R32_SINT, DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT };

			std::vector<NUM_RTV_CHANNELS> m_RenderTargetMasks;
			static constexpr const char* m_TargetSM = "ps_6_6";
			static constexpr const char* m_ShaderExtension = ".ps.hlsl";

			void Reset();

			bool ValidateShaderTypeFromFilepath(const std::string& path) override;

			NUM_RTV_CHANNELS ReflectionMaskToNumComponents(BYTE channelMask);

			bool Reflect(CComPtr<IDxcResult>& compilationResult, CComPtr<IDxcUtils>& utils);

			bool ValidateFormatWithMask(DXGI_FORMAT format, NUM_RTV_CHANNELS numRtvComponents);

		public:
			PixelShader() = default;
			PixelShader(IDxcCompiler3& compiler, const std::string& path);
			bool CompileFromFile(IDxcCompiler3& compiler, const std::string& path) override;
			uint32_t NumRenderTargets() const { return m_RenderTargetMasks.size(); }
			bool ValidateRenderTargetDXGIs(std::span<DXGI_FORMAT> formats);
		};
	}
}