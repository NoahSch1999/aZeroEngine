#include "ComputeShaderPass.hpp"

aZero::Pipeline::ComputeShaderPass::ComputeShaderPass(ComputeShaderPass&& other) noexcept
{
	*this = std::move(other);
}

aZero::Pipeline::ComputeShaderPass& aZero::Pipeline::ComputeShaderPass::operator=(ComputeShaderPass&& other) noexcept
{
	ShaderPassBase::operator=(std::move(other));
	std::swap(m_ThreadGroupCount, other.m_ThreadGroupCount);
	return *this;
}

bool aZero::Pipeline::ComputeShaderPass::CreatePipelineState(ID3D12DeviceX* device, const Description& description, Pipeline::ComputeShader& computeShader, Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineState) const
{
	// todo Impl

	return true;
}

bool aZero::Pipeline::ComputeShaderPass::Compile(ID3D12DeviceX* device, const Description& description, Pipeline::ComputeShader& computeShader)
{
	if (!this->ValidatePassInputs(description))
	{
		DEBUG_PRINT("Validation of pass failed.");
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	if (!ShaderPassBase::CreateRootSignature(device, description, rootSignature, computeShader));
	{
		DEBUG_PRINT("Failed to create root signature.");
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState = nullptr;
	if (!this->CreatePipelineState(device, description, computeShader, pipelineState))
	{
		DEBUG_PRINT("Failed to create pipeline state.");
		return false;
	}

	BindingCombo<BufferBinding> bufferBindings;
	BindingCombo<ConstantBinding> constantBindings;
	ShaderPassBase::GenerateBindings(bufferBindings, constantBindings, computeShader);

	ShaderPassBase::PostCompile(std::move(pipelineState), std::move(rootSignature), std::move(bufferBindings), std::move(constantBindings));

	m_ThreadGroupCount = computeShader.m_ThreadGroupCount;

	return true;
}

void aZero::Pipeline::ComputeShaderPass::Begin(RenderAPI::CommandList& cmdList) const
{
	ShaderPassBase::Begin(cmdList);
	cmdList->SetComputeRootSignature(m_RootSignature.Get());
}