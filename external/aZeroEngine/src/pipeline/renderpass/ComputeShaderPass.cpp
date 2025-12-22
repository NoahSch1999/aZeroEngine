#include "ComputeShaderPass.hpp"

bool aZero::Pipeline::ComputeShaderPass::CreatePipelineState(ID3D12Device* device, const Description& description, Pipeline::ComputeShader& computeShader, ID3D12PipelineState* pipelineState) const
{
	// todo Impl

	return true;
}

bool  aZero::Pipeline::ComputeShaderPass::CreatePipeline(ID3D12Device* device, const Description& description, Pipeline::ComputeShader& computeShader, ID3D12PipelineState* pipelineState, ID3D12RootSignature* rootSignature) const
{
	if (!NewRenderPass::CreateRootSignature(device, description, rootSignature, computeShader));
	{
		DEBUG_PRINT("Failed to create root signature.");
		return false;
	}

	if (!this->CreatePipelineState(device, description, computeShader, pipelineState))
	{
		DEBUG_PRINT("Failed to create pipeline state.");
		return false;
	}

	return true;
}

bool aZero::Pipeline::ComputeShaderPass::Compile(ID3D12Device* device, const Description& description, Pipeline::ComputeShader& computeShader)
{
	if (!this->ValidatePassInputs(description))
	{
		DEBUG_PRINT("Validation of pass failed.");
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	if (!this->CreatePipeline(device, description, computeShader, pipelineState.Get(), rootSignature.Get()))
	{
		DEBUG_PRINT("Failed to compile pass pipeline.");
		return false;
	}

	BindingCombo<BufferBinding> bufferBindings;
	BindingCombo<ConstantBinding> constantBindings;
	NewRenderPass::GenerateBindings(bufferBindings, constantBindings, computeShader);

	NewRenderPass::PostCompile(std::move(pipelineState), std::move(rootSignature), std::move(bufferBindings), std::move(constantBindings));
	// todo Assign local variables

	return true;
}

void aZero::Pipeline::ComputeShaderPass::Bind(RenderAPI::CommandList& cmdList) const
{
	for (const BufferBinding& buffer : m_BufferBindings.m_Bindings)
	{
		// Skip binding bufferslot if no buffer was bound
		if (!buffer.m_Buffer)
		{
			DEBUG_PRINT("A buffer wasn't bound to the pass.");
			continue;
		}

		switch (buffer.m_Type)
		{
		case BindingType::SRV:
			cmdList->SetComputeRootShaderResourceView(buffer.m_Index, buffer.m_Buffer->GetResource()->GetGPUVirtualAddress());
			break;
		case BindingType::UAV:
			cmdList->SetComputeRootUnorderedAccessView(buffer.m_Index, buffer.m_Buffer->GetResource()->GetGPUVirtualAddress());
			break;
		default:
			throw std::runtime_error("Bound buffer doesn't have a valid binding type.");
		}
	}

	for (const ConstantBinding& constant : m_ConstantBindings.m_Bindings)
	{
		cmdList->SetComputeRoot32BitConstants(constant.m_Index, constant.m_NumConstants, constant.m_Data.get(), 0);
	}
}