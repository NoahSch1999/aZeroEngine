#include "VertexShaderPass.hpp"

bool aZero::Pipeline::VertexShaderPass::CreatePipelineState(ID3D12DeviceX* device, const Description& description, Pipeline::VertexShader& vertexShader, std::optional<Pipeline::PixelShader*> pixelShader, Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSignature) const
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc;
	ZeroMemory(&pipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	pipelineStateDesc.pRootSignature = rootSignature.Get();
	pipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	// todo Make this a setting
	DXGI_SAMPLE_DESC sampleDesc{};
	sampleDesc.Count = 1;
	sampleDesc.Quality = 0;
	pipelineStateDesc.SampleDesc = sampleDesc;

	// todo Make this a setting
	D3D12_RASTERIZER_DESC rasterDesc{};
	rasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rasterDesc.FrontCounterClockwise = true;
	pipelineStateDesc.RasterizerState = rasterDesc;

	// todo Make this a setting
	D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pipelineStateDesc.BlendState = blendDesc;

	pipelineStateDesc.InputLayout.NumElements = vertexShader.m_InputElementDescs.size();
	pipelineStateDesc.InputLayout.pInputElementDescs = vertexShader.m_InputElementDescs.data();

	// todo Fix
	pipelineStateDesc.PrimitiveTopologyType = static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(description.m_TopologyType);

	// todo Make this a setting
	pipelineStateDesc.SampleMask = std::numeric_limits<uint32_t>::max();

	if (pixelShader.has_value())
	{
		Pipeline::PixelShader& pixelShaderRef = *pixelShader.value();
		pipelineStateDesc.NumRenderTargets = description.m_RenderTargets.size();

		if (pixelShaderRef.m_RenderTargetMasks.size() != description.m_RenderTargets.size())
		{
			DEBUG_PRINT("Pixel shaders number of render targets doesn't match the descriptions.");
			return false;
		}

		for (int i = 0; i < description.m_RenderTargets.size(); i++)
		{
			const UINT rtvNumComponents = D3D12_PROPERTY_LAYOUT_FORMAT_TABLE::GetNumComponentsInFormat(description.m_RenderTargets.at(i).m_Format);
			const uint32_t expectedNumComponents = pixelShaderRef.m_RenderTargetMasks.at(i);
			if (expectedNumComponents == static_cast<uint32_t>(rtvNumComponents))
			{
				pipelineStateDesc.RTVFormats[i] = description.m_RenderTargets.at(i).m_Format;
			}
			else
			{
				const LPCSTR formatName = D3D12_PROPERTY_LAYOUT_FORMAT_TABLE::GetName(description.m_RenderTargets.at(i).m_Format);
				DEBUG_PRINT("Expected a DXGI_FORMAT with " << expectedNumComponents << " components but " << formatName << " has " << rtvNumComponents << " components");
				return false;
			}
		}

		pipelineStateDesc.PS = {
		reinterpret_cast<BYTE*>(pixelShaderRef.m_CompiledShader->GetBufferPointer()),
		pixelShaderRef.m_CompiledShader->GetBufferSize()
		};
	}

	// TODO: wrong usage of the depth
	//throw;
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	if (description.m_DepthStencil.m_Format == DXGI_FORMAT::DXGI_FORMAT_UNKNOWN)
	{
		depthStencilDesc.DepthEnable = false;
	}
	else
	{
		pipelineStateDesc.DepthStencilState = depthStencilDesc;
		pipelineStateDesc.DSVFormat = description.m_DepthStencil.m_Format;
	}

	pipelineStateDesc.VS = {
		reinterpret_cast<BYTE*>(vertexShader.m_CompiledShader->GetBufferPointer()),
		vertexShader.m_CompiledShader->GetBufferSize()
	};

	const HRESULT res = device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&pipelineState));
	if (FAILED(res))
	{
		DEBUG_PRINT("Failed to create graphics pipelinestate: " + std::to_string(res));
		return false;
	}

	return true;
}

bool aZero::Pipeline::VertexShaderPass::Compile(ID3D12DeviceX* device, const Description& description, Pipeline::VertexShader& vertexShader, std::optional<Pipeline::PixelShader*> pixelShader)
{
	if (!this->ValidatePassInputs(description))
	{
		DEBUG_PRINT("Validation of pass failed.");
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	if (!MultiShaderPass::CreateRootSignature(device, description, vertexShader, pixelShader, rootSignature))
	{
		DEBUG_PRINT("Failed to compile pass root signature.");
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState = nullptr;
	if (!this->CreatePipelineState(device, description, vertexShader, pixelShader, pipelineState, rootSignature))
	{
		DEBUG_PRINT("Failed to compile pass pipeline state.");
		return false;
	}

	BindingCombo<BufferBinding> bufferBindings;
	BindingCombo<ConstantBinding> constantBindings;
	DataStructures::SparseMappedVector<std::string, RenderAPI::Descriptor*> renderTargets;
	MultiShaderPass::GenerateBindings(bufferBindings, constantBindings, renderTargets, description, vertexShader, pixelShader);

	MultiShaderPass::PostCompile(std::move(renderTargets), std::move(pipelineState), std::move(rootSignature), std::move(bufferBindings), std::move(constantBindings));

	m_TopologyType = description.m_TopologyType;

	return true;
}