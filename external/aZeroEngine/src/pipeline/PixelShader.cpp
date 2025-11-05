#include "PixelShader.hpp"

aZero::Pipeline::PixelShader::PixelShader(IDxcCompiler3& compiler, const std::string& path)
{
	this->CompileFromFile(compiler, path);
}

void aZero::Pipeline::PixelShader::Reset()
{
	Shader::Reset();
	m_RenderTargetMasks.clear();
}

bool aZero::Pipeline::PixelShader::ValidateShaderTypeFromFilepath(const std::string& path)
{
	return path.ends_with(m_ShaderExtension);
}

aZero::Pipeline::PixelShader::NUM_RTV_CHANNELS aZero::Pipeline::PixelShader::ReflectionMaskToNumComponents(BYTE channelMask)
{
	if (channelMask == 15)
	{
		return NUM_RTV_CHANNELS::RGBA;
	}
	else if (channelMask == 7)
	{
		return NUM_RTV_CHANNELS::RGB;
	}
	else if (channelMask == 3)
	{
		return NUM_RTV_CHANNELS::RG;
	}
	else if (channelMask == 1)
	{
		return NUM_RTV_CHANNELS::R;
	}
	else
	{
		throw;
	}

	return NUM_RTV_CHANNELS::R;
}

bool aZero::Pipeline::PixelShader::Reflect(CComPtr<IDxcResult>& compilationResult, CComPtr<IDxcUtils>& utils)
{
	Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection;
	if (!Shader::ReflectImpl(compilationResult, utils, D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL, reflection))
	{
		return false;
	}

	D3D12_SHADER_DESC shaderDesc{};
	reflection->GetDesc(&shaderDesc);

	m_RenderTargetMasks.reserve(shaderDesc.OutputParameters);
	for (uint32_t ParamIndex = 0; ParamIndex < shaderDesc.OutputParameters; ParamIndex++)
	{
		D3D12_SIGNATURE_PARAMETER_DESC SignatureParameterDesc{};
		reflection->GetOutputParameterDesc(ParamIndex, &SignatureParameterDesc);
		m_RenderTargetMasks.emplace_back(this->ReflectionMaskToNumComponents(SignatureParameterDesc.Mask));
	}

	return true;
}

bool aZero::Pipeline::PixelShader::ValidateFormatWithMask(DXGI_FORMAT format, NUM_RTV_CHANNELS numRtvComponents)
{
	switch (numRtvComponents)
	{
	case NUM_RTV_CHANNELS::RGBA:
	{
		for (const DXGI_FORMAT& validFormat : m_ValidFormatsRGBA)
		{
			if (format == validFormat)
			{
				return true;
			}
		}
		break;
	}
	case NUM_RTV_CHANNELS::RGB:
	{
		for (const DXGI_FORMAT& validFormat : m_ValidFormatsRGB)
		{
			if (format == validFormat)
			{
				return true;
			}
		}
		break;
	}
	case NUM_RTV_CHANNELS::RG:
	{
		for (const DXGI_FORMAT& validFormat : m_ValidFormatsRG)
		{
			if (format == validFormat)
			{
				return true;
			}
		}
		break;
	}
	case NUM_RTV_CHANNELS::R:
	{
		for (const DXGI_FORMAT& validFormat : m_ValidFormatsR)
		{
			if (format == validFormat)
			{
				return true;
			}
		}
		break;
	}
	}
	return false;
}

bool aZero::Pipeline::PixelShader::CompileFromFile(IDxcCompiler3& compiler, const std::string& path)
{
	if (!this->ValidateShaderTypeFromFilepath(path))
	{
		DEBUG_PRINT("Shader path (" + path + ") isn't valid (path doesn't end with '.ps.hlsl' etc...");
		return false;
	}

	this->Reset();

	CComPtr<IDxcResult> compilationResult;
	CComPtr<IDxcUtils> utils;
	if (!Shader::CompileImpl(compiler, path, m_TargetSM, compilationResult, utils))
	{
		this->Reset();
		return false;
	}

	if (!this->Reflect(compilationResult, utils))
	{
		this->Reset();
		return false;
	}

	return true;
}

bool aZero::Pipeline::PixelShader::ValidateRenderTargetDXGIs(std::span<DXGI_FORMAT> formats)
{
	if (formats.size() != m_RenderTargetMasks.size())
	{
		DEBUG_PRINT("Number of render target formats doesn't match number of pixel shader output render targets.");
		return false;
	}

	for (int rtvFormatIndex = 0; rtvFormatIndex < formats.size(); rtvFormatIndex++)
	{
		if (!this->ValidateFormatWithMask(formats[rtvFormatIndex], m_RenderTargetMasks.at(rtvFormatIndex)))
		{
			DEBUG_PRINT("Input DXGI_FORMAT at index " + std::to_string(rtvFormatIndex) + " wasn't a valid format for the render target which requires " + std::to_string(m_RenderTargetMasks.at(rtvFormatIndex)) + " number of channels.");
		}
	}

	return true;
}