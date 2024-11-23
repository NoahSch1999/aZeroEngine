#pragma once
#include <string>
#include <unordered_map>
#include "Core/AssetTypes/MeshAsset.h"

namespace aZero
{
	class MeshFileCache
	{
	private:
		std::unordered_map<std::string, Asset::MeshAsset> m_Assets;

	public:
		MeshFileCache() = default;

		void LoadFromFile(const std::string& FilePath, ID3D12GraphicsCommandList* CmdList)
		{
			if (FilePath.ends_with(".azMesh")) // Custom mesh file format
			{

			}
			else if (FilePath.ends_with(".fbx"))
			{
				std::vector<Asset::MeshInfo> LoadedFBX;
				const bool IsLoaded = Asset::LoadFBXFile(LoadedFBX, FilePath);
				if (IsLoaded)
				{
					ID3D12Device* Device = nullptr;
					CmdList->GetDevice(IID_PPV_ARGS(&Device));
					for (int MeshIndex = 0; MeshIndex < LoadedFBX.size(); MeshIndex++)
					{
						const Asset::MeshInfo& Mesh = LoadedFBX[MeshIndex];

						if (m_Assets.count(Mesh.Name) > 0)
						{
							// Handle duplicate mesh-names
							throw;
							//
						}

						m_Assets.emplace(Mesh.Name, Asset::MeshAsset(Mesh, CmdList));
					}
				}
			}
		}

		const Asset::MeshAsset* GetAsset(const std::string& Name)
		{
			if (m_Assets.count(Name) > 0)
			{
				return &m_Assets.at(Name);
			}

			return nullptr;
		}

		void RemoveAsset(const std::string& Name)
		{
			if (m_Assets.count(Name) > 0)
			{
				m_Assets.erase(Name);
			}
		}

		// TODO - Implement move and copy constructors / operators
		MeshFileCache(const MeshFileCache&) = delete;
		MeshFileCache(MeshFileCache&&) = delete;
		MeshFileCache operator=(const MeshFileCache&) = delete;
		MeshFileCache& operator=(MeshFileCache&&) = delete;
	};
}