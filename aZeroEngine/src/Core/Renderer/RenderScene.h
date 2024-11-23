#pragma once
#include "Core/Renderer/D3D12Wrap/Resources/PackedGPULookupBuffer.h"
#include "Core/Renderer/D3D12Wrap/Resources/FrameAllocator.h"

namespace aZero
{
	namespace Rendering
	{
		struct PrimitiveRenderData
		{
			DXM::Matrix WorldMatrix;
			int MaterialIndex;
			int MeshIndex;
		};

		struct PointLightRenderData
		{
			DXM::Vector3 Position;
			DXM::Vector3 Color;
			float Range;
		};

		// Make entity have ptr to renderscene? somehow the renderscene has to be reachable from the entity
		class RenderScene
		{
		private:
			D3D12::PackedGPULookupBuffer<PrimitiveRenderData> m_PrimitiveDataGPUBuffer;
			D3D12::PackedGPULookupBuffer<PointLightRenderData> m_PointLightGPUBuffer;

		public:

			template<typename RenderDataType>
			ID3D12Resource* GetRenderBuffer()
			{
				if constexpr (std::is_same_v<RenderDataType, PrimitiveRenderData>)
				{
					return m_PrimitiveDataGPUBuffer.GetResource();
				}
				else if constexpr (std::is_same_v<RenderDataType, PointLightRenderData>)
				{
					return m_PointLightGPUBuffer.GetResource();
				}
				else
				{
					throw;
				}
				return nullptr;
			}

			RenderScene() = default;

			RenderScene(unsigned int NumPrimitives, unsigned int NumPointLights)
			{
				this->Init(NumPrimitives, NumPointLights);
			}

			~RenderScene();

			void Init(unsigned int NumPrimitives, unsigned int NumPointLights)
			{
				m_PrimitiveDataGPUBuffer.Init(100, NumPrimitives);
				m_PointLightGPUBuffer.Init(100, NumPointLights);
			}

			void AddPrimitive(unsigned int Index)
			{
				m_PrimitiveDataGPUBuffer.Register(Index);
			}

			void AddPointLight(unsigned int Index)
			{
				m_PointLightGPUBuffer.Register(Index);
			}

			void RemovePrimitive(unsigned int Index)
			{
				m_PrimitiveDataGPUBuffer.Remove(Index);
			}

			void RemovePointLight(unsigned int Index)
			{
				m_PointLightGPUBuffer.Remove(Index);
			}

			void UpdatePrimitive(unsigned int Index, void* Data, D3D12::FrameAllocator& Allocator, UINT ElementDataOffset = 0, UINT NumBytes = sizeof(PrimitiveRenderData))
			{
				m_PrimitiveDataGPUBuffer.Update(Index, Data, Allocator, ElementDataOffset, NumBytes);
			}

			void UpdatePointLight(unsigned int Index, void* Data, D3D12::FrameAllocator& Allocator, UINT ElementDataOffset = 0, UINT NumBytes = sizeof(PointLightRenderData))
			{
				m_PointLightGPUBuffer.Update(Index, Data, Allocator, ElementDataOffset, NumBytes);
			}

			bool IsPrimitive(unsigned int Index) const
			{
				return m_PrimitiveDataGPUBuffer.IsRegistered(Index);
			}

			bool IsPointLight(unsigned int Index) const
			{
				return m_PointLightGPUBuffer.IsRegistered(Index);
			}
		};
	}
}