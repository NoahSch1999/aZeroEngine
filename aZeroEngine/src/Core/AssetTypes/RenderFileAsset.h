#pragma once
#include <memory>
#include <string>

namespace aZero
{
	namespace Rendering
	{
		class Renderer;
	}

	namespace Asset
	{
		// TODO: Fix potential problem with using the move constructor and staticmeshcomponent...
		template<typename AssetType, typename GPUResourceHandle>
		class RenderFileAsset
		{
			friend Rendering::Renderer; // NOTE: To avoid having the GPUResourceHandle accessible to the public

		protected:
			GPUResourceHandle m_AssetGPUHandle;
			AssetType m_AssetData;

			void RemoveRenderState()
			{
				m_AssetGPUHandle = GPUResourceHandle();
			}

		public:
			RenderFileAsset() = default; 

			RenderFileAsset(const AssetType& Asset)
			{
				m_AssetData = Asset;
			}

			RenderFileAsset(AssetType&& Asset)
			{
				m_AssetData = std::move(std::forward(Asset));
			}

			
			RenderFileAsset(const RenderFileAsset& Other)
			{
				m_AssetData = Other.m_AssetData;
				m_AssetGPUHandle = Other.m_AssetGPUHandle;
			}

			RenderFileAsset(RenderFileAsset&& Other) noexcept
			{
				m_AssetData = std::move(Other.m_AssetData);
				m_AssetGPUHandle = std::move(Other.m_AssetGPUHandle);
			}

			RenderFileAsset& operator=(RenderFileAsset&& Other)
			{
				if (this != &Other)
				{
					m_AssetData = std::move(Other.m_AssetData);
					m_AssetGPUHandle = std::move(Other.m_AssetGPUHandle);
				}

				return *this;
			}

			RenderFileAsset& operator=(const RenderFileAsset& Other)
			{
				m_AssetData = Other.m_AssetData;
				m_AssetGPUHandle = Other.m_AssetGPUHandle;
				return *this;
			}


			const AssetType& GetAssetData() const { return m_AssetData; }
			const GPUResourceHandle& GetGPUAssetHandle() const { return m_AssetGPUHandle; }

			void SetAssetData(AssetType&& Asset)
			{
				m_AssetData = std::move(Asset);

				if (this->HasRenderState())
				{
					this->RemoveRenderState();
				}
			}

			void SetAssetData(const AssetType& Asset)
			{
				m_AssetData = Asset;

				if (this->HasRenderState())
				{
					this->RemoveRenderState();
				}
			}

			virtual bool HasRenderState() const = 0;

			virtual bool LoadFromFile(const std::string& Path) = 0;
		};
	}
}