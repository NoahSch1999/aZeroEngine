#include "TextureAsset.h"
#include "Inc/WICTextureLoader.h"
#include "Inc/DDSTextureLoader.h"

bool aZero::Asset::LoadTexture2DFromFile(const std::string& FilePath, LoadedTextureData& OutData)
{
	OutData = LoadedTextureData();
	const std::wstring FilePathWStr(FilePath.begin(), FilePath.end());

	if (FilePath.ends_with(".dds"))
	{
		OutData.FilePath = FilePath;
		HRESULT Hr = DirectX::LoadDDSTextureFromFile(gDevice.Get(), FilePathWStr.c_str(), OutData.Resource.GetAddressOf(), OutData.Data, OutData.SubresourceData);
		if (FAILED(Hr))
		{
			return false;
		}
	}
	else
	{
		D3D12_SUBRESOURCE_DATA subData;
		HRESULT Hr = DirectX::LoadWICTextureFromFile(gDevice.Get(), FilePathWStr.c_str(), OutData.Resource.GetAddressOf(), OutData.Data, subData);
		if (FAILED(Hr))
		{
			return false;
		}

		OutData.FilePath = FilePath;
		OutData.SubresourceData.push_back(subData);
	}

	return true;
}