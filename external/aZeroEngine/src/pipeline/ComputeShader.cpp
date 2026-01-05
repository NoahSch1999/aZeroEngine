#include "ComputeShader.hpp"

aZero::Pipeline::ComputeShader::ComputeShader(IDxcCompilerX& compiler, const std::string& path)
{
	this->CompileFromFile(compiler, path);
}

void aZero::Pipeline::ComputeShader::Reset()
{
	Shader::Reset();
	m_ThreadsX = 0;
	m_ThreadsY = 0;
	m_ThreadsZ = 0;
}

bool aZero::Pipeline::ComputeShader::ValidateShaderTypeFromFilepath(const std::string& path)
{
	return path.ends_with(m_ShaderExtension);
}

bool aZero::Pipeline::ComputeShader::Reflect(CComPtr<IDxcResult>& compilationResult, CComPtr<IDxcUtils>& utils)
{
	Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection;
	if (!Shader::ReflectImpl(compilationResult, utils, D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL, reflection))
	{
		return false;
	}

	reflection->GetThreadGroupSize(&m_ThreadsX, &m_ThreadsY, &m_ThreadsZ);

	return true;
}

bool aZero::Pipeline::ComputeShader::CompileFromFile(IDxcCompilerX& compiler, const std::string& path)
{
	if (!this->ValidateShaderTypeFromFilepath(path))
	{
		DEBUG_PRINT("Shader path (" + path + ") isn't valid (path doesn't end with '.cs.hlsl' etc...");
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