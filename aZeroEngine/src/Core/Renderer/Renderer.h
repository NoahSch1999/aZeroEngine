#pragma once
#include "Core/Renderer/D3D12Wrap/Commands/CommandQueue.h"
#include "Core/Renderer/D3D12Wrap/Descriptors/DescriptorManager.h"
#include "Core/Caches/ShaderCache.h"
#include "Core/Renderer/D3D12Wrap/Pipeline/Graphics/GraphicsPass.h"
#include "Core/Renderer/D3D12Wrap/Pipeline/Compute/ComputePass.h"
#include "Core/Renderer/D3D12Wrap/Resources/StructuredBuffer.h"
#include "Core/Renderer/D3D12Wrap/Resources/GPUTexture.h"
#include "Core/DataStructures/UniqueIndexList.h"
#include "Core/Renderer/D3D12Wrap/Pipeline/InputLayout.h"
#include "Core/AssetTypes/MeshAsset.h"
#include "Core/AssetTypes/TextureAsset.h"
#include "Core/Caches/MeshCache.h"
#include "Core/Caches/TextureCache.h"
#include "Core/aZeroECSWrap/aZeroECS/aZeroECS.h"
#include "Core/Renderer/D3D12Wrap/Window/Window.h"
#include "Core/Renderer/D3D12Wrap/Resources/PackedGPULookupBuffer.h"
#include "Core/AssetTypes/MaterialAsset.h"
#include "RenderScene.h"

namespace aZero
{
	class Engine;
	class Scene;

	namespace Rendering
	{

		

		#define PRIMITIVE_DATA_WORLDMATRIX_OFFSET 0
		#define PRIMITIVE_DATA_TEXTUREINDEX_OFFSET sizeof(DXM::Matrix)
		#define PRIMITIVE_DATA_MESHINDEX_OFFSET sizeof(DXM::Matrix) + sizeof(int)

		struct PerDrawConstants
		{
			int PrimitiveIndex;
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

		class Renderer
		{
			/*
			JOB:
			Interface between engine and actually rendering
			-Has all the resources needed to render
				-Works scene based, ie. each Scene has its renderer counter part which is rendered independenltly
					-Ie. each scene has the culling pass and render pass done consecutively, but with the render targets merged
						-Allows easy "chunk"/"scene" removal/adding

			-Scene is registered
				-Create render scene
				-Entity is updated, get scene equivalent and update that on renderer
			
			*/
			friend Engine;
			// Definitely Temp stuff!!!!
		public:
			std::set<UINT> m_TempEntities;
			MeshFileCache m_Meshes;
			TextureFileCache m_TextureFileAssets;

		private:
			const UINT64 BUFFERING_COUNT = 3;

			// Primitives
			std::vector<D3D12::FrameAllocator> m_FrameAllocators;

			D3D12::PackedGPULookupBuffer<Asset::BasicMaterialRenderData> m_BasicMaterialGPUBuffer;
			std::unordered_map<int, int> m_BasicMaterialRefCount;

			D3D12::CommandContext m_PackedGPULookupBufferUpdateContext;

			D3D12::Descriptor m_DefaultSamplerDescriptor;

			D3D12::GPUTexture m_FinalRenderSurface;
			D3D12::Descriptor m_FinalRenderSurfaceUAV;
			D3D12::Descriptor m_FinalRenderSurfaceRTV;

			D3D12::GPUTexture m_SceneDepthTexture;
			D3D12::Descriptor m_SceneDepthTextureDSV;

			// Main Rendering Classes
			D3D12::CommandQueue m_GraphicsQueue;
			D3D12::DescriptorManager m_DescriptorManager;
			D3D12::CommandContext m_GraphicsCommandContext;

			// Pipeline Caching
			ShaderCache m_ShaderCache;
			std::unordered_map<std::string, D3D12::GraphicsPass> m_GraphicsPassCache;
			std::unordered_map<std::string, D3D12::ComputePass> m_ComputePassCache;

			DXM::Vector2 m_RenderResolution;
			D3D12_CLEAR_VALUE m_RTVClearColor;
			D3D12_CLEAR_VALUE m_DSVClearColor;

			UINT64 m_FrameIndex = 0;
			UINT64 m_FrameCount = 0;
			UINT64 m_LastPresentSignal;

			ECS::ComponentManagerDecl* m_ComponentManager = nullptr;

			RenderScene m_RenderScene;
			Scene* m_CurrentScene = nullptr;

		private:

			void SetupRenderPipeline();

			void PrepareGPUBuffers();

			void RenderPrimitives();

			void CopyRenderSurfaceToBackBuffer(D3D12::SwapChain& SwapChain);

			void Present(D3D12::SwapChain& SwapChain)
			{
				SwapChain.Present();
				m_LastPresentSignal = m_GraphicsQueue.ForceSignal();
			}

		protected:
			void Init(ECS::ComponentManagerDecl& ComponentManager, const DXM::Vector2& RenderResolution);

			void BeginFrame();

			void Render(D3D12::SwapChain& SwapChain);

			void EndFrame();

			void ChangeRenderResolution(const DXM::Vector2& NewRenderResolution);

			void FlushImmediate() { m_GraphicsQueue.FlushImmediate(); }

		public:
			Renderer() = default;

			~Renderer()
			{
				m_GraphicsQueue.FlushCommands();
			}

			const DXM::Vector2& GetRenderResolution() const { return m_RenderResolution; }

			D3D12::CommandQueue& GetGraphicsQueue() { return m_GraphicsQueue; }

			bool IsElegibleAsPrimitive(const ECS::Entity& Entity)
			{
				if (Entity.GetComponentMask().test(m_ComponentManager->GetComponentBit<ECS::TransformComponent>())
					&& Entity.GetComponentMask().test(m_ComponentManager->GetComponentBit<ECS::StaticMeshComponent>())
					)
				{
					return true;
				}

				return false; // check bitset
			}

			void UploadFullPrimitiveData(const ECS::Entity& Entity, RenderScene& InRenderScene)
			{
				// stage copy into gpu buffer with component data
				PrimitiveRenderData PrimData;

				// Give data
				const ECS::TransformComponent& TransformComp = *m_ComponentManager->GetComponentArray<ECS::TransformComponent>().GetComponent(Entity);
				const ECS::StaticMeshComponent& StaticMeshComp = *m_ComponentManager->GetComponentArray<ECS::StaticMeshComponent>().GetComponent(Entity);
				PrimData.WorldMatrix = TransformComp.GetTransform();
				PrimData.MeshIndex = 2;
				PrimData.MaterialIndex = StaticMeshComp.GetReferenceID();
				//

				InRenderScene.UpdatePrimitive(Entity.GetID(), &PrimData, m_FrameAllocators[m_FrameIndex]);
			}

			void UploadFullPointLightData(const ECS::Entity& Entity, RenderScene& InRenderScene)
			{
				const ECS::PointLightComponent& PointLightComp = *m_ComponentManager->GetComponentArray<ECS::PointLightComponent>().GetComponent(Entity);
				PointLightRenderData RenderData;
				RenderData.Position = PointLightComp.GetPosition();
				RenderData.Color = PointLightComp.GetColor();
				RenderData.Range = PointLightComp.GetRange();

				// Parent light position to the transform component
				if (m_ComponentManager->HasComponent<ECS::TransformComponent>(Entity))
				{
					RenderData.Position += m_ComponentManager->GetComponentArray<ECS::TransformComponent>().GetComponent(Entity)->GetTransform().Translation();
				}

				InRenderScene.UpdatePointLight(Entity.GetID(), &RenderData, m_FrameAllocators[m_FrameIndex]);
			}

			void UploadFullSpotLightData(const ECS::Entity& Entity)
			{
				/*const ECS::SpotLightComponent& SpotLightComp = *m_ComponentManager->GetComponentArray<ECS::SpotLightComponent>().GetComponent(Entity);
				SpotLightRenderData RenderData;
				RenderData.m_Position = SpotLightComp.GetPosition();
				RenderData.m_Color = SpotLightComp.GetColor();
				RenderData.m_Range = SpotLightComp.GetRange();
				RenderData.m_Radius = SpotLightComp.GetRadius();

				m_SpotLightGPUBuffer.Update(Entity.GetID(), &RenderData, m_FrameAllocators[m_FrameIndex]);*/
			}

			void UploadFullDirectionalLightData(const ECS::Entity& Entity)
			{
				/*const ECS::DirectionalLightComponent& DirectionalLightComp = *m_ComponentManager->GetComponentArray<ECS::DirectionalLightComponent>().GetComponent(Entity);
				DirectionalLightRenderData RenderData;
				RenderData.m_Direction = DirectionalLightComp.GetDirection();
				RenderData.m_Color = DirectionalLightComp.GetColor();

				m_DirectionalLightGPUBuffer.Update(Entity.GetID(), &RenderData, m_FrameAllocators[m_FrameIndex]);*/
			}

			template<typename ComponentType>
			void RegisterRenderComponent(const ECS::Entity& Entity)
			{
				if constexpr (std::is_same_v<ComponentType, ECS::TransformComponent>)
				{
					if (this->IsElegibleAsPrimitive(Entity))
					{
						if (!m_RenderScene.IsPrimitive(Entity.GetID()))
						{
							m_RenderScene.AddPrimitive(Entity.GetID());
							this->UploadFullPrimitiveData(Entity, m_RenderScene);
						}
					}
				}
				else if constexpr (std::is_same_v<ComponentType, ECS::StaticMeshComponent>)
				{
					if (this->IsElegibleAsPrimitive(Entity))
					{
						if (!m_RenderScene.IsPrimitive(Entity.GetID()))
						{
							m_RenderScene.AddPrimitive(Entity.GetID());
							this->UploadFullPrimitiveData(Entity, m_RenderScene);
						}
					}
				}
				else if constexpr (std::is_same_v<ComponentType, ECS::PointLightComponent>)
				{
					if (!m_RenderScene.IsPointLight(Entity.GetID()))
					{
						m_RenderScene.AddPointLight(Entity.GetID());

						// Do something so that the renderer knows what indice it is in (ex. add to tree)

						//

						this->UploadFullPointLightData(Entity, m_RenderScene);
					}
				}
				else
				{
					return;
				}
			}

			template<typename ComponentType>
			void UnregisterRenderComponent(const ECS::Entity& Entity)
			{
				if constexpr (std::is_same_v<ComponentType, ECS::TransformComponent>)
				{
					if (m_RenderScene.IsPrimitive(Entity.GetID()))
					{
						m_RenderScene.RemovePrimitive(Entity.GetID());
					}
				}
				else if constexpr (std::is_same_v<ComponentType, ECS::StaticMeshComponent>)
				{
					if (m_RenderScene.IsPrimitive(Entity.GetID()))
					{
						m_RenderScene.RemovePrimitive(Entity.GetID());
					}
				}
				else if constexpr (std::is_same_v<ComponentType, ECS::PointLightComponent>)
				{
					if (m_RenderScene.IsPointLight(Entity.GetID()))
					{
						m_RenderScene.RemovePointLight(Entity.GetID());
						
						// Do something so that the renderer removes what indice it is in (ex. from tree)

						//
					}
				}
				else
				{
					return;
				}
			}

			template<typename ComponentType>
			void UpdateRenderComponent(const ECS::Entity& Entity, const ComponentType& Component)
			{
				// NOTE - If the input is a copy of the component this could easily be a threadsafe function that enqueues the comp data copy on another thread
				if constexpr (std::is_same_v<ComponentType, ECS::TransformComponent>)
				{
					if (m_RenderScene.IsPrimitive(Entity.GetID()))
					{
						m_RenderScene.UpdatePrimitive(Entity.GetID(), (void*)&Component.GetTransform(), m_FrameAllocators.at(m_FrameIndex), PRIMITIVE_DATA_WORLDMATRIX_OFFSET, sizeof(DXM::Matrix));
					}
				}
				else if constexpr (std::is_same_v<ComponentType, ECS::StaticMeshComponent>)
				{
					if (m_RenderScene.IsPrimitive(Entity.GetID()))
					{
						int x = 2;
						m_RenderScene.UpdatePrimitive(Entity.GetID(), &x, m_FrameAllocators.at(m_FrameIndex), PRIMITIVE_DATA_MESHINDEX_OFFSET, sizeof(int));
					}
				}
				else if constexpr (std::is_same_v<ComponentType, ECS::PointLightComponent>)
				{
					if (m_RenderScene.IsPointLight(Entity.GetID()))
					{
						this->UploadFullPointLightData(Entity.GetID(), m_RenderScene);
					}
				}
				else
				{
					return;
				}
			}

			void ForceRegisterMaterial(const Asset::MaterialTemplate<Asset::BasicMaterialRenderData>& Material)
			{
				const int MaterialID = Material.GetID();
				Asset::BasicMaterialRenderData RenderData = Material.GetRenderData();
				m_BasicMaterialGPUBuffer.Register(MaterialID);
				m_BasicMaterialGPUBuffer.Update(MaterialID, &RenderData, m_FrameAllocators[m_FrameIndex]);

				if (m_BasicMaterialRefCount.count(MaterialID) == 0)
				{
					m_BasicMaterialRefCount.emplace(MaterialID, 1);
				}
				else
				{
					m_BasicMaterialRefCount.at(MaterialID)++;
				}
			}

			void ForceUnregisterMaterial(const Asset::MaterialTemplate<Asset::BasicMaterialRenderData>& Material)
			{
				const int MaterialID = Material.GetID();

				if (m_BasicMaterialRefCount.count(MaterialID) > 0)
				{
					const int CurrentRefCount = m_BasicMaterialRefCount.at(MaterialID);
					m_BasicMaterialRefCount.at(MaterialID)--;

					if (CurrentRefCount - 1 == 0)
					{
						m_BasicMaterialGPUBuffer.Remove(MaterialID);
						m_BasicMaterialRefCount.erase(MaterialID);
					}
				}
			}

			void MarkMaterialDirty(const Asset::MaterialTemplate<Asset::BasicMaterialRenderData>& Material)
			{
				if (m_BasicMaterialRefCount.count(Material.GetID()) == 0)
				{
					// No material is referenced, so no use to update (might not even be registered to the renderer)
					return;
				}

				Asset::BasicMaterialRenderData RenderData = Material.GetRenderData();
				m_BasicMaterialGPUBuffer.Update(Material.GetID(), &RenderData, m_FrameAllocators[m_FrameIndex]);
			}

			void SetScene(Scene* InScene)
			{
				m_CurrentScene = InScene;
			}
		};

	}
	extern Rendering::Renderer gRenderer;
}