#pragma once
#include "Core/Renderer/D3D12Wrap/Resources/LinearFrameAllocator.h"
#include "Core/Renderer/D3D12Wrap/Resources/FreelistBuffer.h"
#include "Core/Renderer/RenderAsset.h"

namespace aZero
{
	namespace Rendering
	{
		struct PrimitiveRenderData
		{
			DXM::Matrix WorldMatrix;
			uint32_t MeshEntryIndex;
			uint32_t MaterialIndex;
		};

		enum PRIMITIVE_DATA_OFFSET { 
			WORLDMATRIX = 0, 
			MESHINDEX = sizeof(DXM::Matrix), 
			MATERIALINDEX = sizeof(DXM::Matrix) + sizeof(uint32_t)
		};

		struct PointLightRenderData
		{
			DXM::Vector3 Position;
			DXM::Vector3 Color;
			float Range;
		}; 
		
		struct SpotLightRenderData
		{
			DXM::Vector3 m_Position;
			DXM::Vector3 m_Color;
			float m_Range;
			float m_Radius;
		};

		struct DirectionalLightRenderData
		{
			DXM::Vector3 m_Direction;
			DXM::Vector3 m_Color;
		};

		class RenderScene
		{
		private:
			D3D12::FreelistBuffer m_PrimitiveDataBuffer;

			D3D12::FreelistBuffer m_PointLightBuffer;
			std::unordered_map<uint64_t, DS::FreelistAllocator::AllocationHandle> m_EntityID_To_PointLightHandle; // Temp

		public:
			std::unordered_map<uint64_t, DS::FreelistAllocator::AllocationHandle> m_EntityID_To_PrimitiveHandle; // Temp
			std::unordered_map<uint32_t, std::shared_ptr<Asset::Mesh>> m_Entity_To_Mesh;

			template<typename RenderDataType>
			ID3D12Resource* GetRenderBuffer() const
			{
				if constexpr (std::is_same_v<RenderDataType, PrimitiveRenderData>)
				{
					return m_PrimitiveDataBuffer.GetBuffer().GetResource();
				}
				else if constexpr (std::is_same_v<RenderDataType, PointLightRenderData>)
				{
					return m_PointLightBuffer.GetBuffer().GetResource();
				}
				else
				{
					throw;
				}
				return nullptr;
			}

			RenderScene() = default;

			RenderScene(const RenderScene& Other) = delete;
			RenderScene& operator=(const RenderScene& Other) = delete;

			RenderScene(RenderScene&& Other) noexcept
			{
				m_EntityID_To_PrimitiveHandle = std::move(Other.m_EntityID_To_PrimitiveHandle);
				m_PrimitiveDataBuffer = std::move(Other.m_PrimitiveDataBuffer);

				m_EntityID_To_PointLightHandle = std::move(Other.m_EntityID_To_PointLightHandle);
				m_PointLightBuffer = std::move(Other.m_PointLightBuffer);

				m_Entity_To_Mesh = std::move(Other.m_Entity_To_Mesh);
			}

			RenderScene& operator=(RenderScene&& Other) noexcept
			{
				if (this != &Other)
				{
					m_EntityID_To_PrimitiveHandle = std::move(Other.m_EntityID_To_PrimitiveHandle);
					m_PrimitiveDataBuffer = std::move(Other.m_PrimitiveDataBuffer);

					m_EntityID_To_PointLightHandle = std::move(Other.m_EntityID_To_PointLightHandle);
					m_PointLightBuffer = std::move(Other.m_PointLightBuffer);

					m_Entity_To_Mesh = std::move(Other.m_Entity_To_Mesh);
				}
				return *this;
			}

			RenderScene(ID3D12Device* Device, uint32_t NumPrimitives, uint32_t NumPointLights, std::optional<D3D12::ResourceRecycler*> OptResourceRecycler)
			{
				this->Init(Device, NumPrimitives, NumPointLights, OptResourceRecycler);
			}

			void Init(ID3D12Device* Device, uint32_t NumPrimitives, uint32_t NumPointLights, std::optional<D3D12::ResourceRecycler*> OptResourceRecycler)
			{
				m_PrimitiveDataBuffer.Init(Device, NumPrimitives * sizeof(PrimitiveRenderData), OptResourceRecycler, D3D12::GPUResource::RWPROPERTY::GPUONLY);
				m_PointLightBuffer.Init(Device, NumPointLights * sizeof(PointLightRenderData), OptResourceRecycler, D3D12::GPUResource::RWPROPERTY::GPUONLY);
			}

			void AddPrimitive(uint32_t Index)
			{
				if (m_EntityID_To_PrimitiveHandle.count(Index) == 0)
				{
					DS::FreelistAllocator::AllocationHandle Handle;
					m_PrimitiveDataBuffer.GetAllocation(Handle, sizeof(PrimitiveRenderData));
					m_EntityID_To_PrimitiveHandle.emplace(Index, std::move(Handle));
				}
			}

			void AddPointLight(uint32_t Index)
			{
				if (m_EntityID_To_PointLightHandle.count(Index) == 0)
				{
					DS::FreelistAllocator::AllocationHandle Handle;
					m_PointLightBuffer.GetAllocation(Handle, sizeof(PointLightRenderData));
					m_EntityID_To_PointLightHandle.emplace(Index, std::move(Handle));
				}
			}

			void RemovePrimitive(uint32_t Index)
			{
				if (m_EntityID_To_PrimitiveHandle.count(Index) != 0)
				{
					m_EntityID_To_PrimitiveHandle.erase(Index);
				}
			}

			void RemovePointLight(uint32_t Index)
			{
				if (m_EntityID_To_PointLightHandle.count(Index) != 0)
				{
					m_EntityID_To_PointLightHandle.erase(Index);
				}
			}

			//void Update

			void UpdatePrimitive(uint32_t Index, void* Data, D3D12::LinearFrameAllocator& Allocator, PRIMITIVE_DATA_OFFSET ElementOffset = PRIMITIVE_DATA_OFFSET::WORLDMATRIX, uint32_t NumBytes = sizeof(PrimitiveRenderData))
			{
				if (m_EntityID_To_PrimitiveHandle.count(Index) != 0)
				{
					DS::FreelistAllocator::AllocationHandle& Handle = m_EntityID_To_PrimitiveHandle.at(Index);
					m_PrimitiveDataBuffer.Write(Allocator, Data, Handle.GetStartOffset() + ElementOffset, NumBytes);
				}
			}

			void UpdatePointLight(uint32_t Index, void* Data, D3D12::LinearFrameAllocator& Allocator)
			{
				if (m_EntityID_To_PointLightHandle.count(Index) != 0)
				{
					m_PointLightBuffer.Write(Allocator, m_EntityID_To_PointLightHandle.at(Index), Data);
				}
			}

			bool IsPrimitive(uint64_t Index) const
			{
				return m_EntityID_To_PrimitiveHandle.count(Index) > 0;
			}

			bool IsPointLight(uint64_t Index) const
			{
				return m_EntityID_To_PointLightHandle.count(Index) > 0;
			}
		};
	}
}