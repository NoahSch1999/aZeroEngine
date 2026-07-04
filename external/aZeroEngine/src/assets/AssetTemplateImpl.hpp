#pragma once
#include "renderer/Renderer.hpp"
#include "Assets.hpp"

namespace aZero::Asset
{
	template<typename T>
	using AssetManager = aZero::Asset::AssetManagerT<T, aZero::Asset::Mesh, aZero::Asset::Material, aZero::Asset::Texture>;
}

template<>
inline void aZero::Rendering::Renderer::RegisterOrUpdateAsset<aZero::Asset::Mesh>(aZero::Asset::Mesh& mesh)
{
	// todo Impl update of existing asset
	if (mesh.GetRenderRef().IsValid())
	{
		throw;
	}
	FrameContext& context = this->GetCurrentContext();
	const auto [meshletOffset, vertexOffset] = m_RenderAssetManager->UpdateRenderState(context.GetFrameStagingAllocator(),
		mesh.GetCachedData().m_VertexData.Meshlets, mesh.GetCachedData().m_VertexData.Vertices,
		mesh.GetCachedData().m_VertexData.Positions, mesh.GetCachedData().m_VertexData.MeshletBounds);
	mesh.m_RenderRef.m_MeshletGlobalOffset = meshletOffset;
	mesh.m_RenderRef.m_VertexGlobalOffset = vertexOffset;
}

template<>
inline void aZero::Rendering::Renderer::UnregisterAsset<aZero::Asset::Mesh>(aZero::Asset::Mesh& mesh)
{
	m_RenderAssetManager->RemoveMeshAsset(mesh.GetRenderRef().m_MeshletGlobalOffset);
}

//template<>
//inline void aZero::Scene::Scene::OnAssetErased<Asset::Mesh>(Asset::Mesh& mesh)
//{
//	m_World.defer_begin();
//	m_World.query<Component::Mesh>().each([this, mesh](flecs::entity entity, const Component::Mesh& meshComponent) {
//		if (meshComponent.m_MeshID == mesh.GetRenderRef().m_MeshletGlobalOffset)
//		{
//			entity.remove<Component::Mesh>();
//		}
//		});
//	m_World.defer_end();
//}

template<>
inline void aZero::Rendering::Renderer::RegisterOrUpdateAsset<aZero::Asset::Material>(aZero::Asset::Material& material)
{
	FrameContext& context = this->GetCurrentContext();

	if (!material.m_AlbedoRef->GetRenderRef().IsValid() && !material.m_NormalRef->GetRenderRef().IsValid())
	{
		throw std::invalid_argument("One or more texture-references aren't valid");
	}

	// todo Handle if the texture isnt valid
	uint32_t albedoIndex = material.m_AlbedoRef ? material.m_AlbedoRef->GetRenderRef().DescriptorIndex : 0;
	uint32_t normalIndex = material.m_NormalRef ? material.m_NormalRef->GetRenderRef().DescriptorIndex : 0;

	// todo Maybe call different overloads based on material properties?
	material.m_RenderRef.MaterialIndex = m_RenderAssetManager->UpdateRenderState(context.GetFrameStagingAllocator(), material.m_RenderRef.MaterialIndex, albedoIndex, normalIndex);
}

template<>
inline void aZero::Rendering::Renderer::UnregisterAsset<aZero::Asset::Material>(aZero::Asset::Material& material)
{
	m_RenderAssetManager->RemoveMaterialAsset(material.GetRenderRef().MaterialIndex);
}

//template<>
//inline void aZero::Scene::Scene::OnAssetErased<Asset::Material>(Asset::Material& material)
//{
//	m_World.defer_begin();
//	m_World.query<Component::Mesh>().each(
//		[material](flecs::entity entity, Component::Mesh& mesh) {
//			for (uint32_t i = 0; i < mesh.m_NumSubmeshes; i++)
//			{
//				if (mesh.m_Submeshes[i].m_MaterialID == material.GetRenderRef().MaterialIndex)
//				{
//					entity.remove<Component::Mesh>();
//				}
//			}
//
//		}
//	);
//	m_World.defer_end();
//}

template<>
inline void aZero::Rendering::Renderer::RegisterOrUpdateAsset<aZero::Asset::Texture>(Asset::Texture& texture)
{
	// todo Impl update of existing asset
	if (texture.GetRenderRef().IsValid())
	{
		throw;
	}
	FrameContext& context = this->GetCurrentContext();
	texture.m_RenderRef.DescriptorIndex = m_RenderAssetManager->UpdateRenderState(m_diDevice, context.GetCommandList(), m_ResourceRecycler, m_ResourceHeap,
		texture.GetCachedData().TexelData, texture.GetCachedData().Width, texture.GetCachedData().Height, texture.GetCachedData().Format);
	m_DirectCommandQueue.ExecuteCommandList(context.GetCommandList());
}

template<>
inline void aZero::Rendering::Renderer::UnregisterAsset<aZero::Asset::Texture>(aZero::Asset::Texture& texture)
{
	m_RenderAssetManager->RemoveTextureAsset(texture.GetRenderRef().DescriptorIndex);

	// todo Impl recycle that doesnt force flush of descriptors
	this->FlushRenderCommands();
}