#pragma once
#include <optional>
#include <mutex>

#include "misc/NonCopyable.hpp"
#include "graphics_api/D3D12Include.hpp"
#include "CommandWrapping.hpp"

namespace aZero
{
	namespace RenderAPI
	{
		/*
		The idea behind these classes are the following:
			- Enable the user to easily and safely create resources that can be bound to the renderer
			- Enable the user to defer resource destruction until they seem fit (using the ResourceRecycler class)

		Example of creating and transfering ownership of resources:
			aZero::Engine engine(3, aZero::Helper::GetProjectDirectory() + "/../../../content");

			RenderAPI::ResourceRecycler recycler;

			{ // Buffer creation
				RenderAPI::Buffer::Desc bufferDesc(10, D3D12_HEAP_TYPE_UPLOAD);
				RenderAPI::Buffer buffer(engine.GetDevice(), bufferDesc, &recycler); // Constructor + upload heap + with recycler
				RenderAPI::Buffer other(std::move(buffer)); // Move constructor
				bufferDesc.AccessType = D3D12_HEAP_TYPE_DEFAULT;
				RenderAPI::Buffer triple = RenderAPI::Buffer(engine.GetDevice(), bufferDesc); // Move operator + default heap + no recycler
			} // Resources out of scope => Defer resource destruction until recycler.Clear()

			{ // Texture2D creation
				RenderAPI::Texture2D::Desc textureDesc(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
				RenderAPI::Texture2D texture(engine.GetDevice(), textureDesc, &recycler); // Constructor + with recycler
				RenderAPI::Texture2D other(std::move(texture)); // Move constructor
				RenderAPI::Texture2D triple = RenderAPI::Texture2D(engine.GetDevice(), textureDesc); // Move operator  + no recycler
			} // Resources out of scope => Defer resource destruction until recycler.Clear()

			recycler.Clear();
		*/

		/*
			Class that guarantees that resources created with it (or appended) are alive until Clear() is called.
		*/
		class ResourceRecycler
		{
		private:
			std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_Resources;
			std::mutex m_Lock;
		public:
			ResourceRecycler() = default;
			void Append(Microsoft::WRL::ComPtr<ID3D12Resource>&& resource);
			void Clear();
		};

		/*
			Base class of render api resources.
			If a subclass is given a ResourceRecycler during initialization the underlying ID3D12Resource will be transferred to the ResourceRecycler and destruction will be deferred until ResourceRecycler::Clear() is called.
				Since a resource is non-copyable, this will mean there should only ever be one refcount on the Microsoft::WRL::ComPtr<ID3D12Resource>. If this isn't met, there's no guarantee that the destruction will be done at said time.
			NOTE: The ResourceRecycler cannot have its address changed, and if so, results in undefined behavior since a dangling pointer will be dereferenced.
		*/
		class Resource : public NonCopyable
		{
		protected:
			Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource = nullptr;
			void Init(ID3D12Device* device, ResourceRecycler* diResourceRecycler, const D3D12_RESOURCE_DESC& resourceDesc, D3D12_HEAP_TYPE accessType, const D3D12_CLEAR_VALUE* clearValue);
			void Reset();

		private:
			ResourceRecycler* m_diResourceRecycler = nullptr;

			void Move(Resource& other);
			void OnDestroy();

		public:
			Resource() = default;
			~Resource();
			Resource(Resource&& other) noexcept;
			Resource& operator=(Resource&& other) noexcept;
			ID3D12Resource* GetResource() const { return m_Resource.Get(); }
		};

		/*
			Buffer render api resource.
			m_MappedPtr will point to the cpu accessible memory unless the D3D12_HEAP_TYPE is D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT, in which case it will be nullptr.
		*/
		class Buffer : public Resource
		{
		public:
			struct Desc
			{
				uint64_t NumBytes;
				D3D12_HEAP_TYPE AccessType;
				Desc() = default;
				Desc(uint64_t numBytes, D3D12_HEAP_TYPE accessType)
					:NumBytes(numBytes), AccessType(accessType){}
			};

		private:
			void* m_MappedPtr = nullptr;

			void Move(Buffer& other);

		public:
			Buffer() = default;
			Buffer(ID3D12Device* device, const Desc& desc, std::optional<ResourceRecycler*> opt_diResourceRecycler = std::optional<ResourceRecycler*>{});
			Buffer(Buffer&& other) noexcept;
			Buffer& operator=(Buffer&& other) noexcept;

			void Init(ID3D12Device* device, const Desc& desc, std::optional<ResourceRecycler*> opt_diResourceRecycler = std::optional<ResourceRecycler*>{});
			void Reset();

			bool IsDescValid(const Desc& desc);
			void* GetCPUAccessibleMemory() { return m_MappedPtr; }
		};

		/*
			2D texture render api resource.
			NOTE: If optClearValue is given during initialization the same value should be used during command list clear operations. The clear value can be accessed via Texture2D::GetClearValue().
			NOTE: Texture2D::CreateTransition() will change the internal resource tracking. Once you've called this function you need to transition the resource to the returned state or else the tracking will be incorrect.
		*/
		class Texture2D : public Resource
		{
		public:
			struct Desc
			{
				uint32_t Width = 0;
				uint32_t Height = 0;
				uint32_t MipLevels = 1;
				DXGI_FORMAT Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
				D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

				Desc() = default;
				Desc(uint32_t width, uint32_t height, DXGI_FORMAT format)
					:Width(width), Height(height), Format(format){}
			};

			struct ResourceState
			{
				D3D12_BARRIER_SYNC Sync = D3D12_BARRIER_SYNC::D3D12_BARRIER_SYNC_NONE;
				D3D12_BARRIER_ACCESS Access = D3D12_BARRIER_ACCESS::D3D12_BARRIER_ACCESS_NO_ACCESS;
				D3D12_BARRIER_LAYOUT Layout = D3D12_BARRIER_LAYOUT::D3D12_BARRIER_LAYOUT_COMMON;
			};

		private:
			D3D12_RESOURCE_STATES m_CurrentState;
			ResourceState m_CurrentResourceState;
			std::optional<D3D12_CLEAR_VALUE> m_OptClearValue;

			void Move(Texture2D& other);

		public:
			Texture2D() = default;
			Texture2D(ID3D12Device* device, const Desc& desc, std::optional<ResourceRecycler*> opt_diResourceRecycler = std::optional<ResourceRecycler*>{}, std::optional<D3D12_CLEAR_VALUE> optClearValue = std::optional<D3D12_CLEAR_VALUE>{});
			Texture2D(Texture2D&& other) noexcept;
			Texture2D& operator=(Texture2D&& other) noexcept;

			void Init(ID3D12Device* device, const Desc& desc, std::optional<ResourceRecycler*> opt_diResourceRecycler = std::optional<ResourceRecycler*>{}, std::optional<D3D12_CLEAR_VALUE> optClearValue = std::optional<D3D12_CLEAR_VALUE>{});
			void Reset();

			D3D12_TEXTURE_BARRIER CreateTransition(D3D12_BARRIER_SYNC newSync, D3D12_BARRIER_ACCESS newAccess, D3D12_BARRIER_LAYOUT newLayout);
			D3D12_RESOURCE_BARRIER CreateTransition(D3D12_RESOURCE_STATES newState);

			bool IsDescValid(const Desc& desc) const;
			std::optional<D3D12_CLEAR_VALUE> GetClearValue() const { return m_OptClearValue; }
			ResourceState GetResourceState() const { return m_CurrentResourceState; }
		};

		// todo Remove once enhanced barriers are used
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