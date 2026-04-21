#pragma once
#include <optional>
#include "misc/NonCopyable.hpp"
#include "ResourceRecycler.hpp"
#include "graphics_api/command_recording/CommandList.hpp"

namespace aZero
{
	namespace RenderAPI
	{

		/*
			The idea behind this class and its children are the following :
			- Enable the user to easily and safely create resources that can be bound to the renderer
			- Enable the user to defer resource destruction until they seem fit(using the ResourceRecycler class)

			If a subclass is given a ResourceRecycler during initialization the underlying ID3D12Resource will be transferred to the ResourceRecycler and destruction will be deferred until ResourceRecycler::Clear() is called.
				Since a resource is non-copyable, this will mean there should only ever be one refcount on the Microsoft::WRL::ComPtr<ID3D12Resource>. If this isn't met, there's no guarantee that the destruction will be done at said time.
			NOTE: The ResourceRecycler cannot have its address changed, and if so, results in undefined behavior since a dangling pointer will be dereferenced.
		*/
		class ResourceBase : public NonCopyable
		{
		public:
			ResourceBase() = default;
			ResourceBase(ID3D12DeviceX* device, aZero::RenderAPI::ResourceRecycler* diResourceRecycler, const D3D12_RESOURCE_DESC& resourceDesc, D3D12_HEAP_TYPE accessType, const D3D12_CLEAR_VALUE* clearValue, D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);
			~ResourceBase();
			ResourceBase(ResourceBase&& other) noexcept;
			ResourceBase& operator=(ResourceBase&& other) noexcept;
			ID3D12Resource* GetResource() const { return m_Resource.Get(); }

		protected:
			Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource = nullptr;

		private:
			ResourceRecycler* m_diResourceRecycler = nullptr;
			void OnDestroy();
		};

		struct ResourceTransitionBundles
		{
			D3D12_RESOURCE_STATES StateBefore, StateAfter;
			ID3D12Resource* ResourcePtr;
		};

		inline void TransitionResources(CommandList& cmdList, const std::vector<ResourceTransitionBundles>& bundles)
		{
			std::vector<D3D12_RESOURCE_BARRIER> barriers(bundles.size());
			for (int barrierIndex = 0; barrierIndex < barriers.size(); barrierIndex++)
			{
				barriers[barrierIndex].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barriers[barrierIndex].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				D3D12_RESOURCE_TRANSITION_BARRIER transitionBarrier;
				transitionBarrier.pResource = bundles[barrierIndex].ResourcePtr;
				transitionBarrier.StateBefore = bundles[barrierIndex].StateBefore;
				transitionBarrier.StateAfter = bundles[barrierIndex].StateAfter;
				transitionBarrier.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				barriers[barrierIndex].Transition = transitionBarrier;
			}

			cmdList->ResourceBarrier(barriers.size(), barriers.data());
		}
	}
}