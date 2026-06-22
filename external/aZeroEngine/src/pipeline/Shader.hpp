#pragma once
#include <string_view>
#include <array>
#include "render_api/D3D12Include.hpp"

namespace aZero
{
	namespace Pipeline
	{
		inline std::string GetShaderDirectoryPath() { return PROJECT_DIRECTORY + std::string("shaderSource/"); }
		inline std::string GetShaderDebugDirectoryPath() { return PROJECT_DIRECTORY + std::string("shaderSource/debugOutput/"); }
		inline std::string GetShaderNameFromPath(std::string_view path)
		{
			const size_t lastSlash = path.find_last_of('/');
			if (lastSlash == std::wstring::npos)
			{
				return "";
			}
			std::string_view shaderName = path.substr(lastSlash + 1, path.length() - lastSlash);
			const size_t lastDot = shaderName.find_last_of(".");
			if (lastDot == std::wstring::npos)
			{
				return "";
			}
			return std::string(shaderName.substr(0, lastDot));
		}

		enum EShaderType { VS = 0, PS = 1, AS = 2, MS = 3, CS = 4, INVALID = 1337 };
		enum ERtvChannel { R = 1, RG = 2, RGB = 3, RGBA = 4 };
		inline ERtvChannel ReflectionMaskToNumComponents(BYTE channelMask)
		{
			if (channelMask == 15)
			{
				return ERtvChannel::RGBA;
			}
			else if (channelMask == 7)
			{
				return ERtvChannel::RGB;
			}
			else if (channelMask == 3)
			{
				return ERtvChannel::RG;
			}
			else if (channelMask == 1)
			{
				return ERtvChannel::R;
			}
			else
			{
				throw std::runtime_error("Invalid channel count.");
			}

			return ERtvChannel::R;
		}

		inline DXGI_FORMAT ReflectionMaskToDXGIFormat(BYTE Mask)
		{
			DXGI_FORMAT Format;
			if (Mask == 15)
			{
				Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			else if (Mask == 7)
			{
				Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
			}
			else if (Mask == 3)
			{
				Format = DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
			}
			else if (Mask == 1)
			{
				Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
			}
			else
			{
				Format = DXGI_FORMAT_UNKNOWN;
			}

			return Format;
		}

		class Shader
		{
		public:
			Shader() = default;
			Shader(IDxcCompilerX& compiler, std::string_view path) { this->Compile(compiler, path); }

			EShaderType GetType() const { return m_Type; }
			ID3D12ShaderReflection* GetReflection() const { return m_Reflection.Get(); }
			IDxcBlob* GetBinary() const { return m_CompiledShader.Get(); }

			bool Compile(IDxcCompilerX& compiler, std::string_view path, bool embedDebug = true);

		private:
			EShaderType DeduceShadertype(std::string_view shaderName);
			bool Reflect(IDxcResult* compilationResult, IDxcUtils* utils);

			EShaderType m_Type;
			Microsoft::WRL::ComPtr<ID3D12ShaderReflection> m_Reflection;
			Microsoft::WRL::ComPtr<IDxcBlob> m_CompiledShader;
			static constexpr std::array<const char*, 5> SHADER_TYPE_LUT = { "vs_6_6", "ps_6_6", "as_6_6", "ms_6_6", "cs_6_6" };
		};
	}
}
