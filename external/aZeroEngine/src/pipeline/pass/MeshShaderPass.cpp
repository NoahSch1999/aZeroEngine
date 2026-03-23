#include "MeshShaderPass.hpp"

aZero::Pipeline::MeshShaderPass::MeshShaderPass(MeshShaderPass&& other) noexcept
{
	*this = std::move(other);
}

aZero::Pipeline::MeshShaderPass& aZero::Pipeline::MeshShaderPass::operator=(MeshShaderPass&& other) noexcept
{
	MultiShaderPass::operator=(std::move(other));
	std::swap(m_MSThreadGroups, other.m_MSThreadGroups);
	return *this;
}

bool aZero::Pipeline::MeshShaderPass::CreatePipelineState(ID3D12DeviceX* device, const Description& description, Pipeline::MeshShader& meshShader, std::optional<Pipeline::PixelShader*> pixelShader, Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineState, Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSignature) const
{
	// todo Implement with AS and PS

	struct PSO_STREAM
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE RootSignature;
		//CD3DX12_PIPELINE_STATE_STREAM_AS AS;
		CD3DX12_PIPELINE_STATE_STREAM_MS MS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		//CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencil;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DepthStencilFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RenderTargets;
		CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BlendDesc;
		CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RasterizerState;
		CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
		CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK SampleMask;
	} stream;

	stream.RootSignature = rootSignature.Get();
	stream.MS = {
		reinterpret_cast<BYTE*>(meshShader.m_CompiledShader->GetBufferPointer()),
		meshShader.m_CompiledShader->GetBufferSize()
	};

	DXGI_SAMPLE_DESC sampleDesc{};
	sampleDesc.Count = 1;
	sampleDesc.Quality = 0;
	stream.SampleDesc = sampleDesc;

	// todo Make this a setting
	CD3DX12_RASTERIZER_DESC rasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rasterDesc.FrontCounterClockwise = true;
	stream.RasterizerState = rasterDesc;

	// todo Make this a setting
	CD3DX12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	stream.BlendDesc = blendDesc;

	// todo Make this a setting
	stream.SampleMask = std::numeric_limits<uint32_t>::max();

	if (pixelShader.has_value())
	{
		Pipeline::PixelShader& ps = *pixelShader.value();
		stream.PS = {
		reinterpret_cast<BYTE*>(ps.m_CompiledShader->GetBufferPointer()),
		ps.m_CompiledShader->GetBufferSize()
		};

		// todo Impl
		//pipelineStateDesc.NumRenderTargets = description.m_RenderTargets.size();

		if (ps.NumRenderTargets() != description.m_RenderTargets.size())
		{
			DEBUG_PRINT("Pixel shaders number of render targets doesn't match the descriptions.");
			return false;
		}

		for (int i = 0; i < description.m_RenderTargets.size(); i++)
		{
			const UINT rtvNumComponents = D3D12_PROPERTY_LAYOUT_FORMAT_TABLE::GetNumComponentsInFormat(description.m_RenderTargets.at(i).m_Format);
			const uint32_t expectedNumComponents = ps.m_RenderTargetMasks.at(i);
			if (expectedNumComponents == static_cast<uint32_t>(rtvNumComponents))
			{
				//pipelineStateDesc.RTVFormats[i] = description.m_RenderTargets.at(i).m_Format;
			}
			else
			{
				const LPCSTR formatName = D3D12_PROPERTY_LAYOUT_FORMAT_TABLE::GetName(description.m_RenderTargets.at(i).m_Format);
				DEBUG_PRINT("Expected a DXGI_FORMAT with " << expectedNumComponents << " components but " << formatName << " has " << rtvNumComponents << " components");
				return false;
			}
		}
	}

	// TODO: wrong usage of the depth
	/*CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	if (description.m_DepthStencil.m_Format == DXGI_FORMAT::DXGI_FORMAT_UNKNOWN)
	{
		depthStencilDesc.DepthEnable = false;
	}
	else
	{
		pipelineStateDesc.DepthStencilState = depthStencilDesc;
		pipelineStateDesc.DSVFormat = description.m_DepthStencil.m_Format;
	}*/

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
	streamDesc.pPipelineStateSubobjectStream = &stream;
	streamDesc.SizeInBytes = sizeof(PSO_STREAM);

	const HRESULT psoSucceded = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(pipelineState.GetAddressOf()));
	if (FAILED(psoSucceded))
	{
		DEBUG_PRINT("Failed to create mesh shader pipeline state.");
		return false;
	}

	return true;
}

bool aZero::Pipeline::MeshShaderPass::Compile(ID3D12DeviceX* device, const Description& description, Pipeline::MeshShader& meshShader, /*todo Add amplification shader,*/ std::optional<Pipeline::PixelShader*> pixelShader)
{
	// todo Make part of engine init checks since we only support gpus that support mesh and amplification shaders
	D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureData = {};
	device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureData, sizeof(featureData));
	if (featureData.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
	{
		DEBUG_PRINT("Mesh and amplification shaders are not supported.");
		return false;
	}

	if (!this->ValidatePassInputs(description))
	{
		DEBUG_PRINT("Validation of pass failed.");
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	if (!MultiShaderPass::CreateRootSignature(device, description, meshShader, pixelShader, rootSignature))
	{
		DEBUG_PRINT("Failed to compile pass pipeline.");
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState = nullptr;
	if (!this->CreatePipelineState(device, description, meshShader, pixelShader, pipelineState, rootSignature))
	{
		DEBUG_PRINT("Failed to compile pass pipeline state.");
		return false;
	}

	BindingCombo<BufferBinding> bufferBindings;
	BindingCombo<ConstantBinding> constantBindings;
	DataStructures::SparseMappedVector<std::string, RenderAPI::Descriptor*> renderTargets;
	MultiShaderPass::GenerateBindings(bufferBindings, constantBindings, renderTargets, description, meshShader, pixelShader);

	MultiShaderPass::PostCompile(std::move(renderTargets), pipelineState, rootSignature, std::move(bufferBindings), std::move(constantBindings));

	m_MSThreadGroups = meshShader.GetThreadGroups();

	return true;
}