#include "VertexShaderPass.hpp"

aZero::Pipeline::VertexShaderPass::VertexShaderPass(VertexShaderPass&& other) noexcept
{
	*this = std::move(other);
}

aZero::Pipeline::VertexShaderPass& aZero::Pipeline::VertexShaderPass::operator=(VertexShaderPass&& other) noexcept
{
	MultiShaderPass::operator=(std::move(other));
	std::swap(m_TopologyType, other.m_TopologyType);
	return *this;
}

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
	if (pixelShader.has_value())
	{
		if (!MultiShaderPass::CreateRootSignature(device, description, rootSignature, vertexShader, *pixelShader.value()))
		{
			DEBUG_PRINT("Failed to compile pass root signature.");
			return false;
		}
	}
	else if (!pixelShader.has_value())
	{
		if (!MultiShaderPass::CreateRootSignature(device, description, rootSignature, vertexShader))
		{
			DEBUG_PRINT("Failed to compile pass root signature.");
			return false;
		}
	}

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState = nullptr;
	if (!this->CreatePipelineState(device, description, vertexShader, pixelShader, pipelineState, rootSignature))
	{
		DEBUG_PRINT("Failed to compile pass pipeline state.");
		return false;
	}

	BindingCombo<BufferBinding> bufferBindings;
	BindingCombo<ConstantBinding> constantBindings;
	MultiShaderPass::GenerateBindings(bufferBindings, constantBindings, description, vertexShader, pixelShader);

	MultiShaderPass::PostCompile(std::move(pipelineState), std::move(rootSignature), std::move(bufferBindings), std::move(constantBindings));

	m_TopologyType = description.m_TopologyType;

	return true;
}

void aZero::Pipeline::VertexShaderPass::Begin(RenderAPI::CommandList& cmdList, const RenderAPI::DescriptorHeap& resourceHeap, const RenderAPI::DescriptorHeap& samplerHeap, const std::vector<RenderAPI::Descriptor*>& renderTargets, const std::optional<RenderAPI::Descriptor*>& depthStencilTarget)
{
	MultiShaderPass::Begin(cmdList, resourceHeap, samplerHeap, renderTargets, depthStencilTarget);
	if (m_TopologyType == TopologyType::TRIANGLE)
	{
		cmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
	else if (m_TopologyType == TopologyType::LINE)
	{
		cmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	}
	else if (m_TopologyType == TopologyType::POINT)
	{
		cmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	}
	else
	{
		DEBUG_PRINT("Invalid topology type.");
	}
}