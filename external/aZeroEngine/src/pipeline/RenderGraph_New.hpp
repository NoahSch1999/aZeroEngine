#pragma once
#include "ScenePass.hpp"
#include "ComputePass.hpp"
#include "scene/Scene.hpp"

namespace aZero
{
	namespace Pipeline
	{
		class RenderGraph
		{
		public:
			// TODO: The renderer forwards these arguments (using the current frame buffer for triple-buffered resources)
			bool Execute(D3D12::CommandQueue& cmdQueue, 
			D3D12::CommandContextAllocator::CommandContextHandle& cmdContext, 
			ID3D12DescriptorHeap* resourceHeap,
			ID3D12DescriptorHeap* samplerHeap,
			const Scene::SceneProxy& sceneProxy, 
			D3D12::GPUBuffer& staticMeshBuffer, 
			D3D12::GPUBuffer& pointLightBuffer, 
			D3D12::GPUBuffer& spotLightBuffer, 
			D3D12::GPUBuffer& directionalLightBuffer)
			{
				const uint64_t frameBufferSize = 10000;
				LinearAllocator frameBatchAllocator(staticMeshBuffer.GetCPUAccessibleMemory(), frameBufferSize);

				std::unordered_map<uint32_t, std::unordered_map<uint32_t, Pipeline::ScenePass::StaticMeshBatchDrawData>> batches;

				LinearAllocator framePointLightAllocator(pointLightBuffer.GetCPUAccessibleMemory(), frameBufferSize);
				std::vector<Scene::SceneProxy::PointLight> pointLights;
				
				LinearAllocator frameSpotLightAllocator(spotLightBuffer.GetCPUAccessibleMemory(), frameBufferSize);
				std::vector<Scene::SceneProxy::SpotLight> spotLights;

				LinearAllocator frameDirectionalLightAllocator(directionalLightBuffer.GetCPUAccessibleMemory(), frameBufferSize);
				std::vector<Scene::SceneProxy::DirectionalLight> directionalLights;

				for (const Scene::SceneProxy::Camera& camera : sceneProxy.m_Cameras.GetData())
				{
					for (const Scene::SceneProxy::StaticMesh& staticMeshEntity : sceneProxy.m_StaticMeshes.GetData())
					{
						if (true/* TODO: Perform frustrum culling */)
						{
							Pipeline::ScenePass::StaticMeshInstanceData instanceData;
							instanceData.Transform = staticMeshEntity.m_Transform;
							auto batch = batches[staticMeshEntity.m_MeshIndex][staticMeshEntity.m_MaterialIndex];
							batch.NumVertices = staticMeshEntity.m_NumVertices;
							batch.InstanceData.emplace_back();
						}
					}

					for (const Scene::SceneProxy::PointLight& pointLight : sceneProxy.m_PointLights.GetData())
					{
						if (true/* TODO: Perform frustrum culling */)
						{
							pointLights.emplace_back(pointLight);
						}
					}

					for (const Scene::SceneProxy::SpotLight& spotLight : sceneProxy.m_SpotLights.GetData())
					{
						if (true/* TODO: Perform frustrum culling */)
						{
							spotLights.emplace_back(spotLight);
						}
					}

					LinearAllocator<>::Allocation pointLightAllocation = framePointLightAllocator.Append((void*)pointLights.data(), pointLights.size() * sizeof(decltype(pointLights)::value_type));
					LinearAllocator<>::Allocation spotLightAllocation = frameSpotLightAllocator.Append((void*)spotLights.data(), spotLights.size() * sizeof(decltype(spotLights)::value_type));

					// Fetching all directional lights from the scene since nothing gets culled from it
					LinearAllocator<>::Allocation directionalLightAllocation = frameDirectionalLightAllocator.Append((void*)sceneProxy.m_DirectionalLights.GetData().data(), sceneProxy.m_DirectionalLights.GetData().size() * sizeof(decltype(sceneProxy.m_DirectionalLights.GetData())::value_type));

					ScenePass::LightDrawData lightDrawData;
					lightDrawData.NumPointLights = pointLights.size();
					lightDrawData.NumSpotLights = spotLights.size();
					lightDrawData.NumDirectionalLights = sceneProxy.m_DirectionalLights.GetData().size();

					// TODO: Make this an iteration over RenderPass-subclasses and do dynamic_cast and execute the correct function
					for (std::weak_ptr<RenderPass_New> pass : m_Passes)
					{
						if (pass.expired())
						{
							DEBUG_PRINT("Pass invalid ptr.");
							throw;
						}

						std::shared_ptr<RenderPass_New> passPtr = pass.lock();
						ScenePass* scenePass = dynamic_cast<ScenePass*>(passPtr.get());
						if (scenePass)
						{
							scenePass->Execute(cmdQueue, cmdContext, resourceHeap, samplerHeap, camera, batches, lightDrawData);
							continue;
						}

						ComputePass* computePass = dynamic_cast<ComputePass*>(passPtr.get());
						if (computePass)
						{
							computePass->Execute(cmdQueue, cmdContext, resourceHeap, samplerHeap);
							continue;
						}
					}
				}

				
			}
		private:
			// TODO: Make this into a list which can be ordered etc.
			std::vector<std::weak_ptr<RenderPass_New>> m_Passes;
		}
	}
}