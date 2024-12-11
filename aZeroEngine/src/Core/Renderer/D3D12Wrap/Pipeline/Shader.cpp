#include <iostream>

#include "Shader.h"
#include "DXCompiler/inc/dxcapi.h"

DXGI_FORMAT ReflectionMaskToDXGIFormat(BYTE Mask)
{
	DXGI_FORMAT Format;
	if (Mask == 15)
	{
		Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
	}
	else if (Mask == 7)
	{
		Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	}
	else if (Mask == 3)
	{
		Format = DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
	}
	else if (Mask == 1)
	{
		Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	}
	else
	{
		Format = DXGI_FORMAT_UNKNOWN;
	}

	return Format;
}

bool aZero::D3D12::Shader::ExtractShaderTypeFromFilepath(const std::string& Path, std::string& OutputStr)
{
	if (Path.ends_with(".vs.hlsl"))
	{
		OutputStr = "vs_6_6";
		m_Type = SHADER_TYPE::VS;
	}
	else if (Path.ends_with(".ps.hlsl"))
	{
		OutputStr = "ps_6_6";
		m_Type = SHADER_TYPE::PS;
	}
	else if (Path.ends_with(".cs.hlsl"))
	{
		OutputStr = "cs_6_6";
		m_Type = SHADER_TYPE::CS;
	}
	else
	{
		return false;
	}

	return true;
}

void aZero::D3D12::Shader::GenerateReflectionData(IDxcResult& CompiledShaderResult, 
	IDxcUtils& Utils, 
	std::optional<std::vector<RootConstantOverride>> RootConstOverride,
	std::optional<std::vector<RenderTargetOverride>> RTOverride)
{
	CComPtr<IDxcBlob> ReflectionData = nullptr;
	const HRESULT ReflectionDataOutputRes = CompiledShaderResult.GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&ReflectionData), nullptr);

	if (FAILED(ReflectionDataOutputRes))
	{
		throw std::invalid_argument("Shader::GenerateReflectionData() => Failed to get shader reflection data");
	}

	DxcBuffer ReflectionBuffer;
	ReflectionBuffer.Ptr = ReflectionData->GetBufferPointer();
	ReflectionBuffer.Size = ReflectionData->GetBufferSize();
	ReflectionBuffer.Encoding = 0;

	Microsoft::WRL::ComPtr<ID3D12ShaderReflection> ShaderReflection;
	Utils.CreateReflection(&ReflectionBuffer, IID_PPV_ARGS(ShaderReflection.GetAddressOf()));

	D3D12_SHADER_DESC ShaderDesc{};
	ShaderReflection->GetDesc(&ShaderDesc);

	D3D12_SHADER_VISIBILITY ShaderVis = D3D12_SHADER_VISIBILITY_ALL; // NOTE: Might cause problems if they reference the same binding
	if (m_Type == SHADER_TYPE::VS)
	{
		ShaderVis = D3D12_SHADER_VISIBILITY_VERTEX;

		m_InputElementDescs.reserve(ShaderDesc.InputParameters);
		m_InputElementSemanticNames.reserve(ShaderDesc.InputParameters);
		for (uint32_t ParamIndex = 0; ParamIndex < ShaderDesc.InputParameters; ParamIndex++)
		{
			D3D12_SIGNATURE_PARAMETER_DESC SignatureParameterDesc{};
			ShaderReflection->GetInputParameterDesc(ParamIndex, &SignatureParameterDesc);

			m_InputElementSemanticNames.emplace_back(SignatureParameterDesc.SemanticName);

			m_InputElementDescs.emplace_back(
				D3D12_INPUT_ELEMENT_DESC{
					.SemanticName = m_InputElementSemanticNames.back().c_str(),
					.SemanticIndex = SignatureParameterDesc.SemanticIndex,
					.Format = ReflectionMaskToDXGIFormat(SignatureParameterDesc.Mask),
					.InputSlot = 0u,
					.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
					.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // Might be problem with instanced rendering
					.InstanceDataStepRate = 0u,
				}
			);
		}
	}
	else if (m_Type == SHADER_TYPE::PS)
	{
		m_RenderTargetFormats.reserve(ShaderDesc.OutputParameters);
		for (uint32_t ParamIndex = 0; ParamIndex < ShaderDesc.OutputParameters; ParamIndex++)
		{
			D3D12_SIGNATURE_PARAMETER_DESC SignatureParameterDesc{};
			ShaderReflection->GetOutputParameterDesc(ParamIndex, &SignatureParameterDesc);

			DXGI_FORMAT Format = ReflectionMaskToDXGIFormat(SignatureParameterDesc.Mask);

			if (RTOverride.has_value())
			{
				for (const RenderTargetOverride& Override : RTOverride.value())
				{
					if (Override.TargetSlot == ParamIndex)
					{
						Format = Override.Format;
						break;
					}
				}
			}

			m_RenderTargetFormats.emplace_back(Format);
		}
		ShaderVis = D3D12_SHADER_VISIBILITY_PIXEL;
	}
	else if(m_Type == SHADER_TYPE::CS)
	{
		ShaderVis = D3D12_SHADER_VISIBILITY_ALL;
	}
	else
	{
		throw std::invalid_argument("Shader::GenerateReflectionData() => Invalid shader type");
	}

	m_RootParameters.resize(0);
	for (uint32_t ResourceIndex = 0; ResourceIndex < ShaderDesc.BoundResources; ResourceIndex++)
	{
		D3D12_SHADER_INPUT_BIND_DESC ShaderInputBindDesc{};
		ShaderReflection->GetResourceBindingDesc(ResourceIndex, &ShaderInputBindDesc);

		const std::string ParamName(ShaderInputBindDesc.Name);
		const uint32_t RootParameterIndex = m_RootParameters.size();
		ShaderResourceInfo& Info = m_ResourceNameToInformation[ParamName];
		Info.m_RootIndex = RootParameterIndex;

		switch (ShaderInputBindDesc.Type)
		{
			// TODO: Handle more types?
			case D3D_SIT_CBUFFER:
			{
				bool OverrideAsConstant = false;
				if (RootConstOverride.has_value())
				{
					for (const RootConstantOverride& Override : RootConstOverride.value())
					{
						if (Override.BindingSlot == ShaderInputBindDesc.BindPoint)
						{
							OverrideAsConstant = true;
							break;
						}
					}
				}

				if (OverrideAsConstant)
				{
					ID3D12ShaderReflectionConstantBuffer* ShaderReflectionConstantBuffer = ShaderReflection->GetConstantBufferByIndex(ResourceIndex);
					D3D12_SHADER_BUFFER_DESC ConstantBufferDesc{};
					ShaderReflectionConstantBuffer->GetDesc(&ConstantBufferDesc);

					const D3D12_ROOT_PARAMETER RootParameter
					{
						.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
						.Constants{
							.ShaderRegister = ShaderInputBindDesc.BindPoint,
							.RegisterSpace = ShaderInputBindDesc.Space,
							.Num32BitValues = ConstantBufferDesc.Size / sizeof(uint32_t)
						},
						.ShaderVisibility = ShaderVis
					};
					Info.m_ResourceType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
					m_RootParameters.push_back(RootParameter);
				}
				else
				{
					const D3D12_ROOT_PARAMETER RootParameter
					{
						.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
						.Descriptor{
							.ShaderRegister = ShaderInputBindDesc.BindPoint,
							.RegisterSpace = ShaderInputBindDesc.Space
						},
						.ShaderVisibility = ShaderVis
					};
					Info.m_ResourceType = D3D12_ROOT_PARAMETER_TYPE_CBV;
					m_RootParameters.push_back(RootParameter);
				}
				

				break;
			}
			case D3D_SIT_STRUCTURED:
			{
				const D3D12_ROOT_PARAMETER RootParameter
				{
					.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV,
					.Descriptor{
						.ShaderRegister = ShaderInputBindDesc.BindPoint,
						.RegisterSpace = ShaderInputBindDesc.Space
					},
					.ShaderVisibility = ShaderVis
				};
				Info.m_ResourceType = D3D12_ROOT_PARAMETER_TYPE_SRV;
				m_RootParameters.push_back(RootParameter);
				break;
			}
			case D3D_SIT_UAV_RWSTRUCTURED:
			{
				const D3D12_ROOT_PARAMETER RootParameter
				{
					.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV,
					.Descriptor{
						.ShaderRegister = ShaderInputBindDesc.BindPoint,
						.RegisterSpace = ShaderInputBindDesc.Space
					},
					.ShaderVisibility = ShaderVis
				};
				Info.m_ResourceType = D3D12_ROOT_PARAMETER_TYPE_UAV;
				m_RootParameters.push_back(RootParameter);
				break;
			}
			default:
			{
				throw std::invalid_argument("Shader::GenerateReflectionData() => Invalid resource type used");
			}
		}

	}
}

bool aZero::D3D12::Shader::CompileFromFile(IDxcCompiler3& Compiler, 
	const std::string& FilePath, 
	std::optional<std::vector<RootConstantOverride>> RootConstOverride,
	std::optional<std::vector<RenderTargetOverride>> RTOverride)
{
	this->Reset();

	std::string ShaderTargetTempStr;
	const bool IsShaderType = ExtractShaderTypeFromFilepath(FilePath, ShaderTargetTempStr);

	if (!IsShaderType)
	{
		DEBUG_PRINT("Shader::CompileFromFile() => Shader path (" + FilePath + ") isn't valid (filepath doesn't end with '.vs.hlsl', '.ps.hlsl' etc...");
		return false;
	}

	const std::wstring TargetStr(ShaderTargetTempStr.begin(), ShaderTargetTempStr.end());

	std::vector<LPCWSTR> CompilationArgs;

	const std::wstring FilePathWStr(FilePath.begin(), FilePath.end());
#ifdef _DEBUG
	std::wstring ShaderName(FilePathWStr);
	const size_t LastSlash = ShaderName.find_last_of('/');
	if (LastSlash != std::wstring::npos)
	{
		ShaderName = ShaderName.substr(LastSlash + 1, ShaderName.length() - LastSlash);
	}

	const size_t LastDot = ShaderName.find_last_of(L".");
	const std::wstring PDBName(ShaderName.substr(0, LastDot) + L".pdb");

	CompilationArgs.push_back(ShaderName.c_str());
	CompilationArgs.push_back(L"-Zs");
	CompilationArgs.push_back(L"-Fd");
	CompilationArgs.push_back(PDBName.c_str());
#endif

	CompilationArgs.push_back(L"-Qstrip_debug");
	CompilationArgs.push_back(L"-Qstrip_reflect");

	CompilationArgs.push_back(L"-E");
	CompilationArgs.push_back(L"main");
	CompilationArgs.push_back(L"-T");
	CompilationArgs.push_back(TargetStr.c_str());

	CComPtr<IDxcUtils> Utils;
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&Utils));

	CComPtr<IDxcBlobEncoding> Blob = nullptr;
	const HRESULT FileLoadRes = Utils->LoadFile(FilePathWStr.c_str(), nullptr, &Blob);
	if (FAILED(FileLoadRes))
	{
		DEBUG_PRINT("Shader::CompileFromFile() => Couldn't load shader at path: " + FilePath);
		return false;
	}

	DxcBuffer Source;
	Source.Ptr = Blob->GetBufferPointer();
	Source.Size = Blob->GetBufferSize();
	Source.Encoding = DXC_CP_ACP;

	CComPtr<IDxcIncludeHandler> IncludeHandler;
	Utils->CreateDefaultIncludeHandler(&IncludeHandler);

	CComPtr<IDxcResult> CompilationRes;
	Compiler.Compile(&Source, CompilationArgs.data(), CompilationArgs.size(), IncludeHandler, IID_PPV_ARGS(&CompilationRes));
	
	HRESULT CompilationStatus;
	CompilationRes->GetStatus(&CompilationStatus);
	if (FAILED(CompilationStatus))
	{
		Microsoft::WRL::ComPtr<IDxcBlobUtf8> Errors{};
		CompilationRes->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&Errors), nullptr);
		if (Errors && Errors->GetStringLength() > 0)
		{
			const LPCSTR ErrorMsg = Errors->GetStringPointer();
			DEBUG_PRINT("Shader::CompileFromFile() => " << ErrorMsg);
		}

		return false;
	}

	CComPtr<IDxcBlob> ShaderBinary = nullptr;
	const HRESULT ShaderBinaryOutputRes = CompilationRes->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&ShaderBinary), nullptr);
	if (FAILED(ShaderBinaryOutputRes))
	{
		DEBUG_PRINT("Shader::CompileFromFile() => Failed to get shader binary blob");
		return false;
	}

	m_CompiledShader = ShaderBinary;

	this->GenerateReflectionData(*CompilationRes.p, *Utils.p, RootConstOverride, RTOverride);

	// TODO: Generate pdb file as here: https://simoncoenen.com/blog/programming/graphics/DxcCompiling#:~:text=So%20now%2C%20it%E2%80%99s%20a%20lot%20more%20clear%20and%20easy%20to%20specifically%20get%20parts%20of%20the%20shader.%20Getting%20shader%20PDBs%20is%20done%20as%20follows%3A
	// https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll#reflection:~:text=separate%20reflection%20blobs.-,PDBs,-When%20your%20shader
	
	return true;
}