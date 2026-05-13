#include "RenderPass.hpp"

std::optional<std::reference_wrapper<aZero::NEW_Pipeline::BufferBinding>> aZero::NEW_Pipeline::RenderPass::GetBufferBinding(std::string_view name)
{
	if (auto iter = m_BufferBindings.m_Name_To_Binding.find(name.data()); iter != m_BufferBindings.m_Name_To_Binding.end())
	{
		return m_BufferBindings.m_Bindings[iter->second];
	}
	return {};
}

std::optional<std::reference_wrapper<aZero::NEW_Pipeline::ConstantBinding>> aZero::NEW_Pipeline::RenderPass::GetConstantBinding(std::string_view name)
{
	if (auto iter = m_ConstantBindings.m_Name_To_Binding.find(name.data()); iter != m_ConstantBindings.m_Name_To_Binding.end())
	{
		return m_ConstantBindings.m_Bindings[iter->second];
	}
	return {};
}

bool aZero::NEW_Pipeline::RenderPass::CompileVertexPass(const Desc& desc, ID3D12DeviceX* device, const Shader& vertexShader, std::optional<std::reference_wrapper<const Shader>> pixelShader)
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

	/*if (!this->CreateRootSignature(device, rootParameters))
	{
		return false;
	}*/

	// TODO: Create pipeline

	return true;
}

bool aZero::NEW_Pipeline::RenderPass::CompileMeshletPass(const Desc& desc, ID3D12DeviceX* device, std::optional<std::reference_wrapper<const Shader>> amplificationShader, const Shader& meshShader, std::optional<std::reference_wrapper<const Shader>> pixelShader)
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

	return true;
}

void aZero::NEW_Pipeline::RenderPass::ExtractRootParameters(const Shader& shader, D3D12_SHADER_VISIBILITY shaderVisType, std::vector<D3D12_ROOT_PARAMETER>& inoutRootParams, MappedBufferBindings& inoutBufferBindings, MappedConstantBindings& inoutConstantBindings)
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
			inoutBufferBindings.m_Bindings.emplace_back(ShaderBindingType::CBV, inoutRootParams.size());
		}
		else if (shaderInputBindDesc.Type == D3D_SIT_STRUCTURED)
		{
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
			inoutBufferBindings.m_Bindings.emplace_back(ShaderBindingType::SRV, inoutRootParams.size());
		}
		else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED)
		{
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
			inoutBufferBindings.m_Bindings.emplace_back(ShaderBindingType::UAV, inoutRootParams.size());
		}
		else
		{
			throw std::runtime_error("Invalid resource bindings.");
		}

		inoutRootParams.emplace_back(param);
	}
}

bool aZero::NEW_Pipeline::RenderPass::CreateRootSignature(ID3D12DeviceX* device, const std::vector<D3D12_ROOT_PARAMETER>& rootParams)
{
	// todo Fill in?
	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
	//

	const D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{
		static_cast<UINT>(rootParams.size()),
		rootParams.data(),
		static_cast<UINT>(staticSamplers.size()), staticSamplers.data(),
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

bool aZero::NEW_Pipeline::RenderPass::CreateVertexPipelineState(const Desc& desc, ID3D12DeviceX* device, const Shader& vertexShader, std::optional<std::reference_wrapper<const Shader>> pixelShader)
{
	// TODO: Impl
	return true;
}

bool aZero::NEW_Pipeline::RenderPass::CreateMeshletPipelineState(const Desc& desc, ID3D12DeviceX* device, std::optional<std::reference_wrapper<const Shader>> amplificationShader, const Shader& meshShader, std::optional<std::reference_wrapper<const Shader>> pixelShader)
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
	return true;
}