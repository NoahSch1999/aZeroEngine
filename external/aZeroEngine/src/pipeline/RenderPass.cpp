#include "RenderPass.hpp"
#include "render_api/command_recording/CommandList.hpp"

std::optional<std::reference_wrapper<aZero::Pipeline::BufferBinding>> aZero::Pipeline::RenderPass::GetBufferBinding(std::string_view name)
{
	if (auto iter = m_BufferBindings.m_Name_To_Binding.find(name.data()); iter != m_BufferBindings.m_Name_To_Binding.end())
	{
		return m_BufferBindings.m_Bindings[iter->second];
	}
	return {};
}

std::optional<std::reference_wrapper<aZero::Pipeline::ConstantBinding>> aZero::Pipeline::RenderPass::GetConstantBinding(std::string_view name)
{
	if (auto iter = m_ConstantBindings.m_Name_To_Binding.find(name.data()); iter != m_ConstantBindings.m_Name_To_Binding.end())
	{
		return m_ConstantBindings.m_Bindings[iter->second];
	}
	return {};
}

void aZero::Pipeline::RenderPass::Begin(RenderAPI::CommandList& cmdList)
{
	cmdList->SetPipelineState(m_PipelineState.Get());

	if (m_Type == ERenderPassType::VERTEX || m_Type == ERenderPassType::MESHLET)
	{
		cmdList->SetGraphicsRootSignature(m_RootSignature.Get());

		if (m_Type == ERenderPassType::VERTEX)
		{
			if (m_TopologyType == ETopologyType::TRIANGLE)
			{
				cmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			}
			else if (m_TopologyType == ETopologyType::LINE)
			{
				cmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_LINELIST);
			}
			else if (m_TopologyType == ETopologyType::POINT)
			{
				cmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
			}
			else
			{
				throw std::invalid_argument("Invalid topology type.");
			}
		}
	}
	else if (m_Type == ERenderPassType::COMPUTE)
	{
		cmdList->SetComputeRootSignature(m_RootSignature.Get());
	}
	else
	{
		throw std::invalid_argument("Invalid pass type.");
	}
}

bool aZero::Pipeline::RenderPass::CompileVertexPass(const VertexPassDesc& desc, ID3D12DeviceX* device, const Shader& vertexShader, std::optional<std::reference_wrapper<const Shader>> pixelShader)
{
	if ((pixelShader.has_value() && pixelShader.value().get().GetType() != EShaderType::PS)
		|| vertexShader.GetType() != EShaderType::VS)
	{
		DEBUG_PRINT("Invalid shader input.");
		return false;
	}

	std::vector<D3D12_ROOT_PARAMETER> rootParameters;

	this->ExtractRootParameters(vertexShader, D3D12_SHADER_VISIBILITY_VERTEX, rootParameters, m_BufferBindings, m_ConstantBindings);

	if (pixelShader.has_value())
	{
		this->ExtractRootParameters(pixelShader.value(), D3D12_SHADER_VISIBILITY_PIXEL, rootParameters, m_BufferBindings, m_ConstantBindings);
	}

	if (!this->CreateRootSignature(device, rootParameters))
	{
		return false;
	}

	if (!this->CreateVertexPipelineState(desc, device, vertexShader, pixelShader))
	{
		return false;
	}

	m_Type = ERenderPassType::VERTEX;

	return true;
}

bool aZero::Pipeline::RenderPass::CompileMeshletPass(const MeshletPassDesc& desc, ID3D12DeviceX* device, std::optional<std::reference_wrapper<const Shader>> amplificationShader, const Shader& meshShader, std::optional<std::reference_wrapper<const Shader>> pixelShader)
{
	if ((amplificationShader.has_value() && amplificationShader.value().get().GetType() != EShaderType::AS)
		|| meshShader.GetType() != EShaderType::MS
		|| (pixelShader.has_value() && pixelShader.value().get().GetType() != EShaderType::PS))
	{
		DEBUG_PRINT("Invalid shader input.");
		return false;
	}

	std::vector<D3D12_ROOT_PARAMETER> rootParameters;

	if (amplificationShader.has_value())
	{
		this->ExtractRootParameters(amplificationShader.value(), D3D12_SHADER_VISIBILITY_AMPLIFICATION, rootParameters, m_BufferBindings, m_ConstantBindings);
	}

	this->ExtractRootParameters(meshShader, D3D12_SHADER_VISIBILITY_MESH, rootParameters, m_BufferBindings, m_ConstantBindings);

	if (pixelShader.has_value())
	{
		this->ExtractRootParameters(pixelShader.value(), D3D12_SHADER_VISIBILITY_PIXEL, rootParameters, m_BufferBindings, m_ConstantBindings);
	}

	if (!this->CreateRootSignature(device, rootParameters))
	{
		return false;
	}

	if (!this->CreateMeshletPipelineState(desc, device, amplificationShader, meshShader, pixelShader))
	{
		return false;
	}

	m_Type = ERenderPassType::MESHLET;

	return true;
}

bool aZero::Pipeline::RenderPass::CompileComputePass(ID3D12DeviceX* device, const Shader& computeShader)
{
	if (computeShader.GetType() != EShaderType::CS)
	{
		DEBUG_PRINT("Invalid shader input.");
		return false;
	}

	std::vector<D3D12_ROOT_PARAMETER> rootParameters;
	this->ExtractRootParameters(computeShader, D3D12_SHADER_VISIBILITY_ALL, rootParameters, m_BufferBindings, m_ConstantBindings);
	if (!this->CreateRootSignature(device, rootParameters))
	{
		return false;
	}

	if (!this->CreateComputePipelineState(device, computeShader))
	{
		return false;
	}

	m_Type = ERenderPassType::COMPUTE;

	return true;
}

void aZero::Pipeline::RenderPass::ExtractRootParameters(const Shader& shader, D3D12_SHADER_VISIBILITY shaderVisType, std::vector<D3D12_ROOT_PARAMETER>& inoutRootParams, MappedBufferBindings& inoutBufferBindings, MappedConstantBindings& inoutConstantBindings)
{
	ID3D12ShaderReflection* reflection = shader.GetReflection();
	D3D12_SHADER_DESC shaderDesc{};
	reflection->GetDesc(&shaderDesc);
	for (uint32_t resourceIndex = 0; resourceIndex < shaderDesc.BoundResources; resourceIndex++)
	{
		D3D12_SHADER_INPUT_BIND_DESC shaderInputBindDesc{};
		reflection->GetResourceBindingDesc(resourceIndex, &shaderInputBindDesc);

		const std::string name(shaderInputBindDesc.Name);
		const bool isRootConstant = shaderInputBindDesc.Type == D3D_SIT_CBUFFER && name.ends_with("_CONSTANT");

		if (isRootConstant)
		{
			if (auto iter = inoutConstantBindings.m_Name_To_Binding.find(shaderInputBindDesc.Name); iter != inoutConstantBindings.m_Name_To_Binding.end())
			{
				const auto& binding = inoutConstantBindings.m_Bindings[iter->second];
				D3D12_ROOT_PARAMETER& param = inoutRootParams[binding.GetRootIndex()];
				if (param.Constants.RegisterSpace == shaderInputBindDesc.Space
					&& param.Constants.ShaderRegister == shaderInputBindDesc.BindPoint
					)
				{
					param.ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;
					continue;
				}
			}

			ID3D12ShaderReflectionConstantBuffer* shaderReflectionConstantBuffer = reflection->GetConstantBufferByIndex(resourceIndex);
			D3D12_SHADER_BUFFER_DESC ConstantBufferDesc{};
			shaderReflectionConstantBuffer->GetDesc(&ConstantBufferDesc);

			uint32_t Num32Bit = 0;
			for (int i = 0; i < ConstantBufferDesc.Variables; i++)
			{
				ID3D12ShaderReflectionVariable* variable = shaderReflectionConstantBuffer->GetVariableByIndex(i);
				D3D12_SHADER_VARIABLE_DESC Desc;
				variable->GetDesc(&Desc);
				Num32Bit += Desc.Size / sizeof(uint32_t);
			}

			D3D12_ROOT_PARAMETER param;
			param.Constants.Num32BitValues = Num32Bit;
			param.Constants.ShaderRegister = shaderInputBindDesc.BindPoint;
			param.Constants.RegisterSpace = shaderInputBindDesc.Space;
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			param.ShaderVisibility = shaderVisType;

			inoutConstantBindings.m_Name_To_Binding.emplace(name, inoutConstantBindings.m_Bindings.size());
			inoutConstantBindings.m_Bindings.emplace_back(Num32Bit, inoutRootParams.size());
			inoutRootParams.emplace_back(param);
			continue;
		}

		if (auto iter = inoutBufferBindings.m_Name_To_Binding.find(shaderInputBindDesc.Name); iter != inoutBufferBindings.m_Name_To_Binding.end())
		{
			const auto& binding = inoutBufferBindings.m_Bindings[iter->second];
			D3D12_ROOT_PARAMETER& param = inoutRootParams[binding.GetRootIndex()];

			// Ahh hell nah...
			D3D12_ROOT_PARAMETER_TYPE type = shaderInputBindDesc.Type == D3D_SIT_CBUFFER
				? D3D12_ROOT_PARAMETER_TYPE_CBV : shaderInputBindDesc.Type == D3D_SIT_STRUCTURED ? D3D12_ROOT_PARAMETER_TYPE_SRV
				: D3D12_ROOT_PARAMETER_TYPE_UAV;

			if (param.ParameterType == type
				&& param.Descriptor.RegisterSpace == shaderInputBindDesc.Space
				&& param.Descriptor.ShaderRegister == shaderInputBindDesc.BindPoint
				)
			{
				param.ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;
				continue;
			}
		}

		D3D12_ROOT_PARAMETER param;
		param.ShaderVisibility = shaderVisType;
		param.Descriptor.ShaderRegister = shaderInputBindDesc.BindPoint;
		param.Descriptor.RegisterSpace = shaderInputBindDesc.Space;

		inoutBufferBindings.m_Name_To_Binding.emplace(name, inoutBufferBindings.m_Bindings.size());

		if (shaderInputBindDesc.Type == D3D_SIT_CBUFFER)
		{
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			inoutBufferBindings.m_Bindings.emplace_back(EShaderBindingType::CBV, inoutRootParams.size());
		}
		else if (shaderInputBindDesc.Type == D3D_SIT_STRUCTURED)
		{
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
			inoutBufferBindings.m_Bindings.emplace_back(EShaderBindingType::SRV, inoutRootParams.size());
		}
		else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED)
		{
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
			inoutBufferBindings.m_Bindings.emplace_back(EShaderBindingType::UAV, inoutRootParams.size());
		}
		else
		{
			throw std::runtime_error("Invalid resource bindings.");
		}

		inoutRootParams.emplace_back(param);
	}
}

bool aZero::Pipeline::RenderPass::CreateRootSignature(ID3D12DeviceX* device, const std::vector<D3D12_ROOT_PARAMETER>& rootParams)
{
	const D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{
		static_cast<UINT>(rootParams.size()),
		rootParams.data(),
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		| D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
		| D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED };

	Microsoft::WRL::ComPtr<ID3DBlob> serializeBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

	const HRESULT rsSerializeRes = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializeBlob.GetAddressOf(), errorBlob.GetAddressOf());
	if (FAILED(rsSerializeRes))
	{
		DEBUG_PRINT("Failed to serialize root signature");
		return false;
	}

	const HRESULT rsRes = device->CreateRootSignature(0, serializeBlob->GetBufferPointer(), serializeBlob->GetBufferSize(), IID_PPV_ARGS(m_RootSignature.GetAddressOf()));
	if (FAILED(rsRes))
	{
		DEBUG_PRINT("Failed to create root signature");
		return false;
	}

	return true;
}

bool aZero::Pipeline::RenderPass::CreateVertexPipelineState(const VertexPassDesc& desc, ID3D12DeviceX* device, const Shader& vertexShader, std::optional<std::reference_wrapper<const Shader>> pixelShader)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc;
	ZeroMemory(&pipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	pipelineStateDesc.pRootSignature = m_RootSignature.Get();
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

	D3D12_SHADER_DESC vsReflection{};
	vertexShader.GetReflection()->GetDesc(&vsReflection);
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
	std::vector<std::string> inputElementSemanticNames;
	inputElementSemanticNames.reserve(vsReflection.InputParameters);
	for (uint32_t ParamIndex = 0; ParamIndex < vsReflection.InputParameters; ParamIndex++)
	{
		D3D12_SIGNATURE_PARAMETER_DESC SignatureParameterDesc{};
		vertexShader.GetReflection()->GetInputParameterDesc(ParamIndex, &SignatureParameterDesc);

		inputElementSemanticNames.emplace_back(SignatureParameterDesc.SemanticName);

		inputElementDescs.emplace_back(
			D3D12_INPUT_ELEMENT_DESC{
				.SemanticName = inputElementSemanticNames.back().c_str(),
				.SemanticIndex = SignatureParameterDesc.SemanticIndex,
				.Format = Pipeline::ReflectionMaskToDXGIFormat(SignatureParameterDesc.Mask),
				.InputSlot = desc.InputSlotOverride.size() > ParamIndex ? desc.InputSlotOverride[ParamIndex] : 0u,
				.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
				.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // No way to get this via dxcompiler :(
				.InstanceDataStepRate = 0u,
			}
		);
	}

	pipelineStateDesc.InputLayout.NumElements = inputElementDescs.size();
	pipelineStateDesc.InputLayout.pInputElementDescs = inputElementDescs.data();

	m_TopologyType = desc.TopologyType;
	pipelineStateDesc.PrimitiveTopologyType = static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(m_TopologyType);

	// todo Make this a setting
	pipelineStateDesc.SampleMask = std::numeric_limits<uint32_t>::max();

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	pipelineStateDesc.VS = {
		reinterpret_cast<BYTE*>(vertexShader.GetBinary()->GetBufferPointer()),
		vertexShader.GetBinary()->GetBufferSize()
	};

	if (pixelShader.has_value())
	{
		const Shader& ps = pixelShader.value();

		D3D12_SHADER_DESC shaderDesc{};
		ps.GetReflection()->GetDesc(&shaderDesc);
		if (shaderDesc.OutputParameters != desc.RtvFormats.size())
		{
			return false;
		}

		if (desc.DsvFormat != DXGI_FORMAT::DXGI_FORMAT_UNKNOWN)
		{
			pipelineStateDesc.DSVFormat = desc.DsvFormat;
		}

		pipelineStateDesc.PS = {
			reinterpret_cast<BYTE*>(ps.GetBinary()->GetBufferPointer()),
			ps.GetBinary()->GetBufferSize()
		};

		for (int i = 0; i < desc.RtvFormats.size(); i++)
		{
			pipelineStateDesc.RTVFormats[i] = desc.RtvFormats[i];
		}
		pipelineStateDesc.NumRenderTargets = desc.RtvFormats.size();
	}

	if (desc.DsvFormat == DXGI_FORMAT::DXGI_FORMAT_UNKNOWN)
	{
		depthStencilDesc.DepthEnable = false;
	}
	else
	{
		pipelineStateDesc.DepthStencilState = depthStencilDesc;
		pipelineStateDesc.DSVFormat = desc.DsvFormat;
	}

	const HRESULT res = device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(m_PipelineState.GetAddressOf()));
	if (FAILED(res))
	{
		DEBUG_PRINT("Failed to create graphics pipelinestate: " + std::to_string(res));
		return false;
	}

	m_Type = ERenderPassType::VERTEX;

	return true;
}

bool aZero::Pipeline::RenderPass::CreateMeshletPipelineState(const MeshletPassDesc& desc, ID3D12DeviceX* device, std::optional<std::reference_wrapper<const Shader>> amplificationShader, const Shader& meshShader, std::optional<std::reference_wrapper<const Shader>> pixelShader)
{
	struct PSO_STREAM
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE RootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_AS AS;
		CD3DX12_PIPELINE_STATE_STREAM_MS MS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencil;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DepthStencilFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RenderTargets;
		CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BlendDesc;
		CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RasterizerState;
		CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
		CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK SampleMask;
	} stream = {};

	if (amplificationShader.has_value())
	{
		const Shader& as = amplificationShader.value();
		stream.AS = {
		reinterpret_cast<BYTE*>(as.GetBinary()->GetBufferPointer()),
		as.GetBinary()->GetBufferSize()
		};
	}

	stream.MS = {
		reinterpret_cast<BYTE*>(meshShader.GetBinary()->GetBufferPointer()),
		meshShader.GetBinary()->GetBufferSize()
	};

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	depthStencilDesc.DepthEnable = false;

	if (pixelShader.has_value())
	{
		const Shader& ps = pixelShader.value();

		D3D12_SHADER_DESC shaderDesc{};
		ps.GetReflection()->GetDesc(&shaderDesc);
		if (shaderDesc.OutputParameters != desc.RtvFormats.size())
		{
			return false;
		}

		if (desc.DsvFormat != DXGI_FORMAT::DXGI_FORMAT_UNKNOWN)
		{
			stream.DepthStencilFormat = desc.DsvFormat;
			depthStencilDesc.DepthEnable = true;
		}

		stream.PS = {
			reinterpret_cast<BYTE*>(ps.GetBinary()->GetBufferPointer()),
			ps.GetBinary()->GetBufferSize()
		};

		D3D12_RT_FORMAT_ARRAY rtvs = {};
		rtvs.NumRenderTargets = desc.RtvFormats.size();

		for (int i = 0; i < desc.RtvFormats.size(); i++)
		{
			rtvs.RTFormats[i] = desc.RtvFormats[i];
		}
		stream.RenderTargets = rtvs;
	}

	CD3DX12_DEPTH_STENCIL_DESC finalDsvDesc(depthStencilDesc);
	stream.DepthStencil = finalDsvDesc;

	// todo Make this a setting
	CD3DX12_RASTERIZER_DESC rasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rasterDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterDesc.FrontCounterClockwise = true;
	stream.RasterizerState = rasterDesc;

	// todo Make this a setting
	stream.BlendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	// todo Make this a setting
	stream.SampleMask = std::numeric_limits<uint32_t>::max();

	stream.SampleDesc = { 1, 0 };
	stream.RootSignature = m_RootSignature.Get();

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
	streamDesc.pPipelineStateSubobjectStream = &stream;
	streamDesc.SizeInBytes = sizeof(PSO_STREAM);

	const HRESULT psoSucceded = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(m_PipelineState.GetAddressOf()));
	if (FAILED(psoSucceded))
	{
		DEBUG_PRINT("Failed to create mesh shader pipeline state.");
		return false;
	}

	m_Type = ERenderPassType::MESHLET;

	return true;
}

bool aZero::Pipeline::RenderPass::CreateComputePipelineState(ID3D12DeviceX* device, const Shader& computeShader)
{
	struct PSO_STREAM
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE RootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_CS CS;
	} stream = {};

	stream.RootSignature = m_RootSignature.Get();
	stream.CS = {
		reinterpret_cast<BYTE*>(computeShader.GetBinary()->GetBufferPointer()),
		computeShader.GetBinary()->GetBufferSize()
	};

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
	streamDesc.pPipelineStateSubobjectStream = &stream;
	streamDesc.SizeInBytes = sizeof(PSO_STREAM);

	const HRESULT psoSucceded = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(m_PipelineState.GetAddressOf()));
	if (FAILED(psoSucceded))
	{
		DEBUG_PRINT("Failed to create compute shader pipeline state.");
		return false;
	}

	return true;
}