#include "MeshShader.hpp"

aZero::Pipeline::MeshShader::MeshShader(IDxcCompilerX& compiler, const std::string& path)
{
	this->CompileFromFile(compiler, path);
}

void aZero::Pipeline::MeshShader::Reset()
{
	Shader::Reset();
	m_ThreadGroupCount = ThreadGroup();
}

bool aZero::Pipeline::MeshShader::ValidateShaderTypeFromFilepath(const std::string& path)
{
	return path.ends_with(m_ShaderExtension);
}

bool aZero::Pipeline::MeshShader::Reflect(CComPtr<IDxcResult>& compilationResult, CComPtr<IDxcUtils>& utils)
{
	Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection;
	if (!Shader::ReflectImpl(compilationResult, utils, D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_MESH, reflection))
	{
		return false;
	}

	reflection->GetThreadGroupSize(&m_ThreadGroupCount.x, &m_ThreadGroupCount.y, &m_ThreadGroupCount.z);
	return true;
}

bool aZero::Pipeline::MeshShader::CompileFromFile(IDxcCompilerX& compiler, const std::string& path)
{
	if (!this->ValidateShaderTypeFromFilepath(path))//
	{
		DEBUG_PRINT("Shader path (" + path + ") isn't valid (path doesn't end with '.ms.hlsl' etc...");
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