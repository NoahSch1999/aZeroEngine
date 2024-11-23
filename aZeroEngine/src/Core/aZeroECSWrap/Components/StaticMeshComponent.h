#pragma once
#include "Core/D3D12Core.h"
#include "Core/AssetTypes/MaterialAsset.h"

namespace aZero
{
	namespace ECS
	{
		class StaticMeshComponent
		{
		private:
			std::shared_ptr<Asset::MaterialTemplate<Asset::BasicMaterialRenderData>> m_MaterialPtr;

		public:
			std::string m_AssetNameRef; // Temp

			StaticMeshComponent() = default;
			StaticMeshComponent(const std::string& AssetName, std::shared_ptr<Asset::MaterialTemplate<Asset::BasicMaterialRenderData>> MaterialPtr = nullptr)
			{
				m_MaterialPtr = MaterialPtr;
				m_AssetNameRef = AssetName;
			}

			int GetReferenceID() const { return m_MaterialPtr != nullptr ? m_MaterialPtr->GetID() : 0; }

			const Asset::BasicMaterialRenderData& GetRenderData() const 
			{
				if (m_MaterialPtr != nullptr)
				{
					return m_MaterialPtr->GetRenderData();
				}
				return Asset::BasicMaterialRenderData();
			}
		};
	}
}