#include <fstream>

#include "Shader.h"

void aZero::D3D12::Shader::Initialize(const std::string& srcFilePath)
{
	if (!m_compiledShader)
	{
		// TODO - Call CompileFromFile and store all the necessary data in the maps etc...
		// CompileFromFile(srcFilePath);
		m_compiledShader = LoadBlobFromFile(srcFilePath);
	}
}

Microsoft::WRL::ComPtr<ID3DBlob> aZero::D3D12::LoadBlobFromFile(const std::string& FilePath)
{
	std::ifstream fin(FilePath, std::ios::binary);
	if (fin.is_open())
	{
		fin.seekg(0, std::ios_base::end);
		std::streampos size = fin.tellg();
		fin.seekg(0, std::ios_base::beg);

		Microsoft::WRL::ComPtr<ID3DBlob> blob;
		HRESULT hr = D3DCreateBlob(size, blob.GetAddressOf());
		if (FAILED(hr))
			throw;

		fin.read((char*)blob->GetBufferPointer(), size);
		fin.close();
		return blob;
	}

	return nullptr;
}