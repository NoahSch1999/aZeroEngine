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

bool aZero::Pipeline::ComputeShaderPass::CreatePipelineState(ID3D12DeviceX* device, const Description& description, Pipeline::ComputeShader& computeShader, Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature) const
{
	struct PSO_STREAM
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE RootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_CS CS;
	} stream = {};

	stream.RootSignature = rootSignature.Get();
	stream.CS = {
	reinterpret_cast<BYTE*>(computeShader.m_CompiledShader->GetBufferPointer()),
	computeShader.m_CompiledShader->GetBufferSize()
	};

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
	streamDesc.pPipelineStateSubobjectStream = &stream;
	streamDesc.SizeInBytes = sizeof(PSO_STREAM);

	const HRESULT psoSucceded = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(pipelineState.GetAddressOf()));
	if (FAILED(psoSucceded))
	{
		DEBUG_PRINT("Failed to create compute shader pipeline state.");
		return false;
	}

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
	if (!ShaderPassBase::CreateRootSignature(device, description, rootSignature, computeShader))
	{
		DEBUG_PRINT("Failed to create root signature.");
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState = nullptr;
	if (!this->CreatePipelineState(device, description, computeShader, pipelineState, rootSignature))
	{
		DEBUG_PRINT("Failed to create pipeline state.");
		return false;
	}

	BindingCombo<BufferBinding> bufferBindings;
	BindingCombo<ConstantBinding> constantBindings;
	ShaderPassBase::GenerateBindings(bufferBindings, constantBindings, computeShader);

	ShaderPassBase::PostCompile(pipelineState, rootSignature, std::move(bufferBindings), std::move(constantBindings));

	m_ThreadGroupCount = computeShader.m_ThreadGroupCount;

	return true;
}

void aZero::Pipeline::ComputeShaderPass::Begin(RenderAPI::CommandList& cmdList, const RenderAPI::DescriptorHeap& resourceHeap, const RenderAPI::DescriptorHeap& samplerHeap) const
{
	ShaderPassBase::Begin(cmdList);
	std::array<ID3D12DescriptorHeap*, 2> heaps{ resourceHeap.Get(), samplerHeap.Get() };
	cmdList->SetDescriptorHeaps(heaps.size(), heaps.data());
	cmdList->SetComputeRootSignature(m_RootSignature.Get());
}