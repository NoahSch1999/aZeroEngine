#pragma once
#include "graphics_api/resource/buffer/Buffer.hpp"
#include "graphics_api/resource/texture/Texture2D.hpp"
#include "graphics_api/descriptor/DescriptorHeap.hpp"
#include "graphics_api/command_recording/CommandList.hpp"
#include "graphics_api/command_recording/CommandQueue.hpp"
#include "LinearAllocator.hpp"

namespace aZero
{
	namespace Rendering
	{
		// New wrappers and frame context
		struct FrameContext : public NonCopyable
		{
			// todo Remove these and replace with a gpu-only buffer that is owned by the scene, but accessed in the renderer only, and that contains all the scene's primitive data
			//		This buffer should be updated via a compute shader that takes all entity updates done through UpdateRenderState() and copies them to the buffer in vram
			//		So there needs to be a dedicated staging buffer, per primitive type, that the compute shader reads from.
			//		Then there needs to be a dedicated vram-only buffer, per primitive, that the compute shader writes to.
			// todo Create shader resource views for them and access them bindlessly in the shaders
			RenderAPI::Buffer m_StaticMeshBuffer;
			RenderAPI::Descriptor m_StaticMeshDescriptor;

			RenderAPI::Buffer m_PointLightBuffer;
			RenderAPI::Descriptor m_PointLightDescriptor;

			RenderAPI::Buffer m_SpotLightBuffer;
			RenderAPI::Descriptor m_SpotLightDescriptor;

			RenderAPI::Buffer m_DirectionalLightBuffer;
			RenderAPI::Descriptor m_DirectionalLightDescriptor;
			//

			// Commandlist stuff
			RenderAPI::CommandList m_DirectCmdList;
			RenderAPI::CommandList m_CopyCmdList;
			RenderAPI::CommandList m_ComputeCmdList;
			//

			// Per-frame linear staging allocator that stores allocations and can record them at a later point
			LinearAllocator<> m_FrameAllocator;

			struct FrameAllocation
			{
				// TODO: Make it so LinearAllocator has the option to allocate without using templates for the handle 
				//				This is to allow arbitrary allocation sizes that can be stored inside std::vector<FrameAllocation> m_FrameAllocations;
				LinearAllocator<>::Handle<int> allocation;
				uint32_t dstOffset;
				RenderAPI::Buffer* buffer;
			};
			std::vector<FrameAllocation> m_FrameAllocations;
			RenderAPI::Buffer m_FrameBuffer;

			template<typename T>
			void AddAllocation(const T& data, RenderAPI::Buffer& buffer, uint32_t dstOffset)
			{
				FrameAllocation allocation;
				//allocation.allocation = m_FrameAllocator.Allocate(sizeof(T));
				allocation.buffer = &buffer;
				allocation.dstOffset = dstOffset;
				m_FrameAllocations.emplace_back(allocation);
			}

			// !note To be executed before rendering a scene since it might contain asset upload data and other things
			void RecordFrameAllocations(RenderAPI::CommandList& cmdList)
			{
				for (const FrameAllocation& allocation : m_FrameAllocations)
				{
					//cmdList->CopyBufferRegion(allocation.buffer->GetResource(), allocation.dstOffset, m_FrameBuffer.GetResource(), allocation.allocation, allocation.allocation.Size);
				}
			}
			//

			uint64_t m_FrameCompleteSignal; // Latest signal that is related to the context - Should be used to find when the frame context is available again

			// Update with this for each new commandlist that is executed during the frame
			void SetLatestSignal(uint64_t signal)
			{
				m_FrameCompleteSignal = signal;
			}

			// Used when we wanna check if we should render the next frame
			// todo Change this once several different kinds of queues are used
			bool IsReady(RenderAPI::CommandQueue& directQueue)
			{
				// !note Currently stalls cpu if the next frame context isnt ready - Might wanna change it to return early to enable the game-loop to make another lap
				return directQueue.WaitForSignal(m_FrameCompleteSignal, true);
			}

			FrameContext() = default;
			FrameContext(ID3D12DeviceX* device, RenderAPI::DescriptorHeap& resourceHeap, RenderAPI::ResourceRecycler& recycler)
			{
				this->Init(device, resourceHeap, recycler);
			}

			FrameContext(FrameContext&&) noexcept {

			}
			FrameContext& operator=(FrameContext&&) noexcept = default;

			void Init(ID3D12DeviceX* device, RenderAPI::DescriptorHeap& resourceHeap, RenderAPI::ResourceRecycler& recycler)
			{
				const uint32_t frameBufferSize = 1000000;
				const RenderAPI::Buffer::Desc frameBufferDesc(frameBufferSize, D3D12_HEAP_TYPE_UPLOAD);
				m_FrameBuffer.Init(device, frameBufferDesc, &recycler);
				m_FrameAllocator = LinearAllocator<>((std::byte*)m_FrameBuffer.GetCPUAccessibleMemory(), frameBufferSize);

				const uint32_t primitiveBufferSize = 1000000;
				const RenderAPI::Buffer::Desc primitiveBufferDesc(primitiveBufferSize, D3D12_HEAP_TYPE_UPLOAD);
				m_StaticMeshBuffer.Init(device, primitiveBufferDesc, &recycler);
				m_PointLightBuffer.Init(device, primitiveBufferDesc, &recycler);
				m_SpotLightBuffer.Init(device, primitiveBufferDesc, &recycler);
				m_DirectionalLightBuffer.Init(device, primitiveBufferDesc, &recycler);

				m_StaticMeshDescriptor = resourceHeap.CreateDescriptor();
				m_PointLightDescriptor = resourceHeap.CreateDescriptor();
				m_SpotLightDescriptor = resourceHeap.CreateDescriptor();
				m_DirectionalLightDescriptor = resourceHeap.CreateDescriptor();

				m_DirectCmdList = RenderAPI::CommandList(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
				m_CopyCmdList = RenderAPI::CommandList(device, D3D12_COMMAND_LIST_TYPE_COPY);
				m_ComputeCmdList = RenderAPI::CommandList(device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
			};

			// Used on engine frame beginning
			void Begin()
			{
				//m_FrameAllocator.Reset();
				m_DirectCmdList.ClearCommandBuffer();
				m_CopyCmdList.ClearCommandBuffer();
				m_ComputeCmdList.ClearCommandBuffer();
			}
		};

	}
}