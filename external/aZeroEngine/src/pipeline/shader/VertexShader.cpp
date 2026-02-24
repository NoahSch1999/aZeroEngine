#include "VertexShader.hpp"

aZero::Pipeline::VertexShader::VertexShader(IDxcCompilerX& compiler, const std::string& path)
{
	this->CompileFromFile(compiler, path);
}

void aZero::Pipeline::VertexShader::Reset()
{
	Shader::Reset();
	m_InputElementDescs.clear();
	m_InputElementSemanticNames.clear();
}

bool aZero::Pipeline::VertexShader::ValidateShaderTypeFromFilepath(const std::string& path)
{
	return path.ends_with(m_ShaderExtension);
}

DXGI_FORMAT aZero::Pipeline::VertexShader::ReflectionMaskToDXGIFormat(BYTE Mask)
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

bool aZero::Pipeline::VertexShader::Reflect(CComPtr<IDxcResult>& compilationResult, CComPtr<IDxcUtils>& utils)
{
	Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection;
	if (!Shader::ReflectImpl(compilationResult, utils, D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_VERTEX, reflection))
	{
		return false;
	}

	D3D12_SHADER_DESC shaderDesc{};
	reflection->GetDesc(&shaderDesc);
	m_InputElementDescs.reserve(shaderDesc.InputParameters);
	m_InputElementSemanticNames.reserve(shaderDesc.InputParameters);
	for (uint32_t ParamIndex = 0; ParamIndex < shaderDesc.InputParameters; ParamIndex++)
	{
		D3D12_SIGNATURE_PARAMETER_DESC SignatureParameterDesc{};
		reflection->GetInputParameterDesc(ParamIndex, &SignatureParameterDesc);

		m_InputElementSemanticNames.emplace_back(SignatureParameterDesc.SemanticName);

		m_InputElementDescs.emplace_back(
			D3D12_INPUT_ELEMENT_DESC{
				.SemanticName = m_InputElementSemanticNames.back().c_str(),
				.SemanticIndex = SignatureParameterDesc.SemanticIndex,
				.Format = this->ReflectionMaskToDXGIFormat(SignatureParameterDesc.Mask),
				.InputSlot = 0u,
				.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
				.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // No way to get this via dxcompiler :(
				.InstanceDataStepRate = 0u,
			}
			);
	}

	return true;
}

bool aZero::Pipeline::VertexShader::CompileFromFile(IDxcCompilerX& compiler, const std::string& path)
{
	if (!this->ValidateShaderTypeFromFilepath(path))//
	{
		DEBUG_PRINT("Shader path (" + path + ") isn't valid (path doesn't end with '.vs.hlsl' etc...");
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