#include "MeshShaderPass.hpp"

bool aZero::Pipeline::MeshShaderPass::CreatePipelineState(ID3D12Device* device, const Description& description, Pipeline::MeshShader& meshShader, std::optional<Pipeline::PixelShader*> pixelShader, ID3D12PipelineState* pipelineState, ID3D12RootSignature* rootSignature) const
{
	// todo Impl

	return true;
}
bool aZero::Pipeline::MeshShaderPass::Compile(ID3D12Device* device, const Description& description, Pipeline::MeshShader& meshShader, std::optional<Pipeline::PixelShader*> pixelShader)
{
	if (!this->ValidatePassInputs(description))
	{
		DEBUG_PRINT("Validation of pass failed.");
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	if (!MultiShaderPass::CreatePipeline(device, description, meshShader, pixelShader, pipelineState.Get(), rootSignature.Get()))
	{
		DEBUG_PRINT("Failed to compile pass pipeline.");
		return false;
	}

	BindingCombo<BufferBinding> bufferBindings;
	BindingCombo<ConstantBinding> constantBindings;
	std::unordered_map<std::string, Rendering::RenderSurface*> renderTargets;
	MultiShaderPass::GenerateBindings(bufferBindings, constantBindings, renderTargets, description, meshShader, pixelShader);

	MultiShaderPass::PostCompile(std::move(renderTargets), pipelineState, rootSignature, std::move(bufferBindings), std::move(constantBindings));
	// todo Assign local variables

	return true;
}