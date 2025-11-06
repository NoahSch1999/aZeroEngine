#include "Shader.hpp"
#include "misc/HelperFunctions.hpp"
#include <filesystem>

void aZero::Pipeline::Shader::Reset()
{
	m_CompiledShader = nullptr;
	m_ResourceNameToInformation.clear();
	m_RootParameters.clear();
}

bool aZero::Pipeline::Shader::CompileImpl(IDxcCompiler3& compiler, const std::string& path, const std::string& targetSM, CComPtr<IDxcResult>& compilationResult, CComPtr<IDxcUtils>& utils)
{
	std::vector<LPCWSTR> compilationArgs;

	const std::wstring filePathWStr(path.begin(), path.end());
#if USE_DEBUG
	std::wstring shaderName(filePathWStr);
	const size_t lastSlash = shaderName.find_last_of('/');
	if (lastSlash != std::wstring::npos)
	{
		shaderName = shaderName.substr(lastSlash + 1, shaderName.length() - lastSlash);
	}

	const size_t lastDot = shaderName.find_last_of(L".");
	const std::wstring pdbName(shaderName.substr(0, lastDot) + L".pdb");

	compilationArgs.push_back(shaderName.c_str());
	compilationArgs.push_back(DXC_ARG_DEBUG);
	compilationArgs.push_back(L"-Fd");
	compilationArgs.push_back(pdbName.c_str());
	compilationArgs.push_back(L"-Od");
#else
	compilationArgs.push_back(L"-O0");
#endif

	compilationArgs.push_back(L"-Qstrip_debug");
	compilationArgs.push_back(L"-Qstrip_reflect");

	compilationArgs.push_back(L"-E");
	compilationArgs.push_back(L"main");
	compilationArgs.push_back(L"-T");
	const std::wstring wStrTargetSM(targetSM.begin(), targetSM.end());
	compilationArgs.push_back(wStrTargetSM.c_str());

	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));

	CComPtr<IDxcBlobEncoding> blob = nullptr;
	const HRESULT fileLoadRes = utils->LoadFile(filePathWStr.c_str(), nullptr, &blob);
	if (FAILED(fileLoadRes))
	{
		DEBUG_PRINT("Couldn't load shader at path: " + path);
		return false;
	}

	DxcBuffer source;
	source.Ptr = blob->GetBufferPointer();
	source.Size = blob->GetBufferSize();
	source.Encoding = DXC_CP_ACP;

	CComPtr<IDxcIncludeHandler> includeHandler;
	utils->CreateDefaultIncludeHandler(&includeHandler);

	compiler.Compile(&source, compilationArgs.data(), compilationArgs.size(), includeHandler, IID_PPV_ARGS(&compilationResult));

	HRESULT compilationStatus;
	compilationResult->GetStatus(&compilationStatus);
	if (FAILED(compilationStatus))
	{
		Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors{};
		compilationResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0)
		{
			const LPCSTR errorMsg = errors->GetStringPointer();
			DEBUG_PRINT(errorMsg);
		}

		return false;
	}

	CComPtr<IDxcBlob> shaderBinary = nullptr;
	const HRESULT shaderBinaryOutputRes = compilationResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBinary), nullptr);
	if (FAILED(shaderBinaryOutputRes))
	{
		DEBUG_PRINT("Failed to get shader binary blob");
		return false;
	}

#if USE_DEBUG
	Microsoft::WRL::ComPtr<IDxcBlob> debugData;
	Microsoft::WRL::ComPtr<IDxcBlobUtf16> debugDataPath;
	const HRESULT pdbRes = compilationResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(debugData.GetAddressOf()), debugDataPath.GetAddressOf());
	if (FAILED(pdbRes))
	{
		DEBUG_PRINT("Failed to get pdb data");
		return false;
	}

	const std::string projectPath = Helper::GetDebugProjectDirectory();
	const std::wstring shaderPath(debugDataPath->GetStringPointer());
	std::string outputPath(projectPath + "/shaderDebugOutput/" + std::string(shaderPath.begin(), shaderPath.end()));
	std::filesystem::path dir = std::filesystem::path(outputPath).parent_path();
	std::filesystem::create_directories(dir);

	std::fstream file(outputPath, std::ios::out | std::ios::trunc | std::ios::binary);
	if (file.is_open())
	{
		file.write((char*)debugData->GetBufferPointer(), debugData->GetBufferSize());
		file.close();
	}
#endif

	this->m_CompiledShader = shaderBinary;
	return true;
}

bool aZero::Pipeline::Shader::ReflectImpl(CComPtr<IDxcResult>& compilationResult, CComPtr<IDxcUtils>& utils, D3D12_SHADER_VISIBILITY shaderVisibility, Microsoft::WRL::ComPtr<ID3D12ShaderReflection>& reflection)
{
	CComPtr<IDxcBlob> reflectionData = nullptr;
	const HRESULT reflectionDataOutputRes = compilationResult.p->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionData), nullptr);

	if (FAILED(reflectionDataOutputRes))
	{
		throw std::invalid_argument("Shader::GenerateReflectionData() => Failed to get shader reflection data");
	}

	DxcBuffer reflectionBuffer;
	reflectionBuffer.Ptr = reflectionData->GetBufferPointer();
	reflectionBuffer.Size = reflectionData->GetBufferSize();
	reflectionBuffer.Encoding = 0;

	Microsoft::WRL::ComPtr<ID3D12ShaderReflection> shaderReflection;
	utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(shaderReflection.GetAddressOf()));

	D3D12_SHADER_DESC shaderDesc{};
	shaderReflection->GetDesc(&shaderDesc);

	// NOTE: shaderVisibility being ALL or specific shader might cause problems if they reference the same binding

	m_RootParameters.resize(0);
	for (uint32_t ResourceIndex = 0; ResourceIndex < shaderDesc.BoundResources; ResourceIndex++)
	{
		D3D12_SHADER_INPUT_BIND_DESC ShaderInputBindDesc{};
		shaderReflection->GetResourceBindingDesc(ResourceIndex, &ShaderInputBindDesc);

		const std::string ParamName(ShaderInputBindDesc.Name);
		const uint32_t RootParameterIndex = m_RootParameters.size();
		ShaderResourceInfo& Info = m_ResourceNameToInformation[ParamName];
		Info.m_RootIndex = RootParameterIndex;

		switch (ShaderInputBindDesc.Type)
		{
		case D3D_SIT_CBUFFER:
		{
			ID3D12ShaderReflectionConstantBuffer* shaderReflectionConstantBuffer = shaderReflection->GetConstantBufferByIndex(ResourceIndex);
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

			const D3D12_ROOT_PARAMETER RootParameter
			{
				.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
				.Constants{
					.ShaderRegister = ShaderInputBindDesc.BindPoint,
					.RegisterSpace = ShaderInputBindDesc.Space,
					.Num32BitValues = Num32Bit
				},
				.ShaderVisibility = shaderVisibility
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
				.ShaderVisibility = shaderVisibility
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
				.ShaderVisibility = shaderVisibility
			};
			Info.m_ResourceType = D3D12_ROOT_PARAMETER_TYPE_UAV;
			m_RootParameters.push_back(RootParameter);
			break;
		}
		default:
		{
			return false;
		}
		}
	}

	reflection = shaderReflection;
	return true;
}
