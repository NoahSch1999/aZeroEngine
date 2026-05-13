#include "ShaderNEW.hpp"

#include <filesystem>
#include <fstream>

bool aZero::NEW_Pipeline::Shader::Reflect(IDxcResult* compilationResult, IDxcUtils* utils)
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

bool aZero::NEW_Pipeline::Shader::Compile(IDxcCompilerX& compiler, std::string_view path)
{
	if (!path.ends_with("hlsl"))
	{
		return false;
	}

	const std::string shaderName = GetShaderNameFromPath(path);
	EShaderType type = this->DeduceShadertype(shaderName);

	std::vector<LPCWSTR> compilationArgs;

#if	USE_DEBUG
	std::wstring wShaderName(shaderName.begin(), shaderName.end());
	compilationArgs.push_back(wShaderName.c_str());
	compilationArgs.push_back(DXC_ARG_DEBUG);
	compilationArgs.push_back(L"-Qembed_debug");
	compilationArgs.push_back(L"-Fd");
	std::wstring wShaderNamePDB = wShaderName + L".pdb";
	compilationArgs.push_back(wShaderNamePDB.c_str());
	compilationArgs.push_back(L"-Od");
#else
		compilationArgs.push_back(L"-Qstrip_debug");
	compilationArgs.push_back(L"-O3");
#endif

		const std::string targetSM(SHADER_TYPE_LUT[type]);
	compilationArgs.push_back(L"-E");
	compilationArgs.push_back(L"main");
	compilationArgs.push_back(L"-T");
	const std::wstring wTargetSM(targetSM.begin(), targetSM.end());
	compilationArgs.push_back(wTargetSM.c_str());

	Microsoft::WRL::ComPtr<IDxcUtils> utils; // Lazy ahhh using this...
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> blob = nullptr;
	const std::wstring wPath(path.begin(), path.end());
	const HRESULT fileLoadRes = utils->LoadFile(wPath.c_str(), nullptr, &blob);
	if (FAILED(fileLoadRes))
	{
		const std::string error(path);
		DEBUG_PRINT(std::string("Couldn't load shader at path: ") + error);
		return false;
	}

	DxcBuffer source;
	source.Ptr = blob->GetBufferPointer();
	source.Size = blob->GetBufferSize();
	source.Encoding = DXC_CP_ACP;

	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;
	utils->CreateDefaultIncludeHandler(&includeHandler);

	// Adds "-I" is used to declare include directories
	const std::string shaderPathDir = "-I " + NEW_Pipeline::GetShaderDirectoryPath();
	const std::wstring wShaderPathDir(shaderPathDir.begin(), shaderPathDir.end());
	compilationArgs.push_back(wShaderPathDir.c_str());

	Microsoft::WRL::ComPtr<IDxcResult> compilationResult;
	compiler.Compile(&source, compilationArgs.data(), compilationArgs.size(), includeHandler.Get(), IID_PPV_ARGS(&compilationResult));

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
		std::string outputPath(NEW_Pipeline::GetShaderDebugDirectoryPath() + std::string(shaderPath.begin(), shaderPath.end()));
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

aZero::NEW_Pipeline::EShaderType aZero::NEW_Pipeline::Shader::DeduceShadertype(std::string_view shaderName)
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