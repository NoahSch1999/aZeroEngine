#include "Shader.hpp"

#include <filesystem>
#include <fstream>

bool aZero::Pipeline::Shader::Reflect(IDxcResult* compilationResult, IDxcUtils* utils)
{
	Microsoft::WRL::ComPtr<IDxcBlob> reflectionData = nullptr;
	const HRESULT reflectionDataOutputRes = compilationResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(reflectionData.GetAddressOf()), nullptr);

	if (FAILED(reflectionDataOutputRes))
	{
		DEBUG_PRINT("Failed to get shader reflection data");
		return false;
	}

	DxcBuffer reflectionBuffer;
	reflectionBuffer.Ptr = reflectionData->GetBufferPointer();
	reflectionBuffer.Size = reflectionData->GetBufferSize();
	reflectionBuffer.Encoding = 0;

	Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection;
	const HRESULT reflectionRes = utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(reflection.GetAddressOf()));
	if (FAILED(reflectionRes))
	{
		DEBUG_PRINT("Failed to reflect shader");
		return false;
	}

	m_Reflection = reflection;

	return true;
}

bool aZero::Pipeline::Shader::Compile(IDxcCompilerX& compiler, std::string_view path, bool embedDebug)
{
	if (!path.ends_with("hlsl"))
	{
		return false;
	}

	std::wstring shaderPath(path.begin(), path.end());
	EShaderType type = this->DeduceShadertype(GetShaderNameFromPath(path));

	std::vector<LPCWSTR> compilationArgs;
	
#ifdef USE_DEBUG
		std::wstring shaderName(shaderPath);
	const size_t lastSlash = shaderName.find_last_of('/');
	if (lastSlash != std::wstring::npos)
	{
		shaderName = shaderName.substr(lastSlash + 1, shaderName.length() - lastSlash);
	}

	const size_t lastDot = shaderName.find_last_of(L".");
	const std::wstring pdbName(shaderName.substr(0, lastDot) + L".pdb");

	compilationArgs.push_back(shaderName.c_str());
	compilationArgs.push_back(DXC_ARG_DEBUG);
	compilationArgs.push_back(L"-Qembed_debug");
	compilationArgs.push_back(L"-Fd");
	compilationArgs.push_back(pdbName.c_str());
	compilationArgs.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);
#else
	if (embedDebug) 
	{
		compilationArgs.push_back(DXC_ARG_DEBUG);
		compilationArgs.push_back(L"-Qembed_debug");
	}
	else 
	{
		compilationArgs.push_back(L"-Qstrip_debug");
	}
	compilationArgs.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
#endif


	compilationArgs.push_back(L"-E");
	compilationArgs.push_back(L"main");
	compilationArgs.push_back(L"-T");

	std::string target(SHADER_TYPE_LUT[type]);
	const std::wstring wStrTargetSM(target.begin(), target.end());
	compilationArgs.push_back(wStrTargetSM.c_str());

	Microsoft::WRL::ComPtr<IDxcUtils> utils;
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));

	Microsoft::WRL::ComPtr<IDxcBlobEncoding> blob = nullptr;
	const HRESULT fileLoadRes = utils->LoadFile(shaderPath.c_str(), nullptr, &blob);
	if (FAILED(fileLoadRes))
	{
		std::string filePath(path);
		DEBUG_PRINT("Couldn't load shader at path: " + filePath);
		return false;
	}

	DxcBuffer source;
	source.Ptr = blob->GetBufferPointer();
	source.Size = blob->GetBufferSize();
	source.Encoding = DXC_CP_ACP;

	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;
	utils->CreateDefaultIncludeHandler(&includeHandler);

	// Adds "-I" is used to declare include directories
	const std::string projectDir(PROJECT_DIRECTORY);
	const std::wstring projectDirW(projectDir.begin(), projectDir.end());
	const std::wstring shaderPathDir = projectDirW + L"shaderSource/";
	compilationArgs.push_back(L"-I ");
	compilationArgs.push_back(shaderPathDir.c_str());

	Microsoft::WRL::ComPtr<IDxcResult> compilationResult;
	compiler.Compile(&source, compilationArgs.data(), compilationArgs.size(), includeHandler.Get(), IID_PPV_ARGS(&compilationResult));
	
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

	Microsoft::WRL::ComPtr<IDxcBlob> shaderBinary = nullptr;
	const HRESULT shaderBinaryOutputRes = compilationResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBinary), nullptr);
	if (FAILED(shaderBinaryOutputRes))
	{
		DEBUG_PRINT("Failed to get shader binary blob");
		return false;
	}


#if	USE_DEBUG
	Microsoft::WRL::ComPtr<IDxcBlob> debugData;
	Microsoft::WRL::ComPtr<IDxcBlobUtf16> debugDataPath;
	const HRESULT pdbRes = compilationResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(debugData.GetAddressOf()), debugDataPath.GetAddressOf());
	if (SUCCEEDED(pdbRes))
	{
		const std::wstring shaderPath(debugDataPath->GetStringPointer());
		std::string outputPath(Pipeline::GetShaderDebugDirectoryPath() + std::string(shaderPath.begin(), shaderPath.end()));
		std::filesystem::path dir = std::filesystem::path(outputPath).parent_path();
		std::filesystem::create_directories(dir);

		std::fstream file(outputPath, std::ios::out | std::ios::trunc | std::ios::binary);
		if (file.is_open())
		{
			file.write((char*)debugData->GetBufferPointer(), debugData->GetBufferSize());
			file.close();
		}
	}
	else
	{
		DEBUG_PRINT("Failed to get pdb data");
	}
#endif

	if (!this->Reflect(compilationResult.Get(), utils.Get()))
	{
		return false;
	}

	m_CompiledShader = shaderBinary;
	m_Type = type;

	return true;
}

aZero::Pipeline::EShaderType aZero::Pipeline::Shader::DeduceShadertype(std::string_view shaderName)
{
	if (shaderName.ends_with("vs"))
	{
		return EShaderType::VS;
	}
	else if (shaderName.ends_with("as"))
	{
		return EShaderType::AS;
	}
	else if (shaderName.ends_with("ms"))
	{
		return EShaderType::MS;
	}
	else if (shaderName.ends_with("ps"))
	{
		return EShaderType::PS;
	}
	else if (shaderName.ends_with("cs"))
	{
		return EShaderType::CS;
	}

	return EShaderType::INVALID;
}