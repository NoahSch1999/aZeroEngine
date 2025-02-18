#include <iostream>

#include "Shader.hpp"
#include "graphics_api/DXCompilerInclude.hpp"

#if USE_DEBUG
#include <fstream>
#include <algorithm>
#endif

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

uint32_t ReflectionMaskToNumComponents(BYTE Mask)
{
	if (Mask == 15)
	{
		return 4;
	}
	else if (Mask == 7)
	{
		return 3;
	}
	else if (Mask == 3)
	{
		return 2;
	}
	else if (Mask == 1)
	{
		return 1;
	}
		
	return std::numeric_limits<uint32_t>::max();
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

void aZero::D3D12::Shader::GenerateReflectionData(IDxcResult& CompiledShaderResult, IDxcUtils& Utils)
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
					.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, // No way to get this via dxcompiler :(
					.InstanceDataStepRate = 0u,
				}
			);
		}
	}
	else if (m_Type == SHADER_TYPE::PS)
	{
		m_RenderTargetMasks.reserve(ShaderDesc.OutputParameters);
		for (uint32_t ParamIndex = 0; ParamIndex < ShaderDesc.OutputParameters; ParamIndex++)
		{
			D3D12_SIGNATURE_PARAMETER_DESC SignatureParameterDesc{};
			ShaderReflection->GetOutputParameterDesc(ParamIndex, &SignatureParameterDesc);
			m_RenderTargetMasks.emplace_back(ReflectionMaskToNumComponents(SignatureParameterDesc.Mask));
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
			case D3D_SIT_CBUFFER:
			{
				ID3D12ShaderReflectionConstantBuffer* ShaderReflectionConstantBuffer = ShaderReflection->GetConstantBufferByIndex(ResourceIndex);
				D3D12_SHADER_BUFFER_DESC ConstantBufferDesc{};
				ShaderReflectionConstantBuffer->GetDesc(&ConstantBufferDesc);

				uint32_t Num32Bit = 0;
				for (int i = 0; i < ConstantBufferDesc.Variables; i++)
				{
					ID3D12ShaderReflectionVariable* Variable = ShaderReflectionConstantBuffer->GetVariableByIndex(i);
					D3D12_SHADER_VARIABLE_DESC Desc;
					Variable->GetDesc(&Desc);
					Num32Bit += Desc.Size / sizeof(uint32_t);
				}

				const D3D12_ROOT_PARAMETER RootParameter
				{
					.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
					.Constants{
						.ShaderRegister = ShaderInputBindDesc.BindPoint,
						.RegisterSpace = ShaderInputBindDesc.Space,
						.Num32BitValues = Num32Bit
					},
					.ShaderVisibility = ShaderVis
				};
				Info.m_Num32BitConstants = Num32Bit;
				Info.m_ResourceType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
				m_RootParameters.push_back(RootParameter);

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

bool aZero::D3D12::Shader::CompileFromFile(IDxcCompiler3& Compiler, const std::string& ShaderFilePath)
{
	this->Reset();

	const std::string FilePath = ShaderFilePath;
	std::string ShaderTargetTempStr;
	const bool IsShaderType = ExtractShaderTypeFromFilepath(FilePath, ShaderTargetTempStr);

	if (!IsShaderType)
	{
		DEBUG_PRINT("Shader path (" + FilePath + ") isn't valid (filepath doesn't end with '.vs.hlsl', '.ps.hlsl' etc...");
		return false;
	}

	const std::wstring TargetStr(ShaderTargetTempStr.begin(), ShaderTargetTempStr.end());

	std::vector<LPCWSTR> CompilationArgs;

	const std::wstring FilePathWStr(FilePath.begin(), FilePath.end());
#if USE_DEBUG
	std::wstring ShaderName(FilePathWStr);
	const size_t LastSlash = ShaderName.find_last_of('/');
	if (LastSlash != std::wstring::npos)
	{
		ShaderName = ShaderName.substr(LastSlash + 1, ShaderName.length() - LastSlash);
	}

	const size_t LastDot = ShaderName.find_last_of(L".");
	const std::wstring PDBName(ShaderName.substr(0, LastDot) + L".pdb");

	CompilationArgs.push_back(ShaderName.c_str());
	CompilationArgs.push_back(DXC_ARG_DEBUG);
	CompilationArgs.push_back(L"-Fd");
	CompilationArgs.push_back(PDBName.c_str());
	CompilationArgs.push_back(L"-Od");
#else
	CompilationArgs.push_back(L"-O0");
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
		DEBUG_PRINT("Couldn't load shader at path: " + FilePath);
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
			DEBUG_PRINT(ErrorMsg);
		}

		return false;
	}

	CComPtr<IDxcBlob> ShaderBinary = nullptr;
	const HRESULT ShaderBinaryOutputRes = CompilationRes->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&ShaderBinary), nullptr);
	if (FAILED(ShaderBinaryOutputRes))
	{
		DEBUG_PRINT("Failed to get shader binary blob");
		return false;
	}

#if USE_DEBUG
	Microsoft::WRL::ComPtr<IDxcBlob> DebugData;
	Microsoft::WRL::ComPtr<IDxcBlobUtf16> DebugDataPath;
	const HRESULT PDBRes = CompilationRes->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(DebugData.GetAddressOf()), DebugDataPath.GetAddressOf());
	if (FAILED(PDBRes))
	{
		DEBUG_PRINT("Failed to get pdb data");
		return false;
	}

	const std::string ProjectPath = Helper::GetDebugProjectDirectory();
	const std::wstring ShaderPath(DebugDataPath->GetStringPointer());
	std::string OutputPath(ProjectPath + "/shaderDebugOutput/" + std::string(ShaderPath.begin(), ShaderPath.end()));

	std::fstream File(OutputPath, std::ios::out | std::ios::trunc | std::ios::binary);
	if(File.is_open())
	{
		File.write((char*)DebugData->GetBufferPointer(), DebugData->GetBufferSize());
		File.close();
	}
#endif

	m_CompiledShader = ShaderBinary;

	this->GenerateReflectionData(*CompilationRes.p, *Utils.p);

	return true;
}