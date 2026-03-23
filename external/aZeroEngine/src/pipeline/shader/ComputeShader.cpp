#include "ComputeShader.hpp"

aZero::Pipeline::ComputeShader::ComputeShader(IDxcCompilerX& compiler, const std::string& path)
{
	this->CompileFromFile(compiler, path);
}

aZero::Pipeline::ComputeShader::ComputeShader(ComputeShader&& other) noexcept
{
	*this = std::move(other);
}

aZero::Pipeline::ComputeShader& aZero::Pipeline::ComputeShader::operator=(ComputeShader&& other) noexcept
{
	Shader::operator=(std::move(other));
	std::swap(m_ThreadGroupCount, other.m_ThreadGroupCount);
	return *this;
}

bool aZero::Pipeline::ComputeShader::ValidateShaderTypeFromFilepath(const std::string& path)
{
	return path.ends_with(m_ShaderExtension);
}

bool aZero::Pipeline::ComputeShader::Reflect(Microsoft::WRL::ComPtr<IDxcResult>& compilationResult, Microsoft::WRL::ComPtr<IDxcUtils>& utils)
{
	Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection;
	if (!Shader::ReflectImpl(compilationResult, utils, D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL, reflection))
	{
		return false;
	}

	reflection->GetThreadGroupSize(&m_ThreadGroupCount.x, &m_ThreadGroupCount.y, &m_ThreadGroupCount.y);

	return true;
}

bool aZero::Pipeline::ComputeShader::CompileFromFile(IDxcCompilerX& compiler, const std::string& path)
{
	if (!this->ValidateShaderTypeFromFilepath(path))
	{
		DEBUG_PRINT("Shader path (" + path + ") isn't valid (path doesn't end with '.cs.hlsl' etc...");
		return false;
	}

	Microsoft::WRL::ComPtr<IDxcResult> compilationResult;
	Microsoft::WRL::ComPtr<IDxcUtils> utils;
	if (!Shader::CompileImpl(compiler, path, m_TargetSM, compilationResult, utils))
	{
		return false;
	}

	if (!this->Reflect(compilationResult, utils))
	{
		return false;
	}

	return true;
}