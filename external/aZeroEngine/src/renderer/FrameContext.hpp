#pragma once
#include "graphics_api/resource/buffer/Buffer.hpp"
#include "graphics_api/resource/texture/Texture2D.hpp"
#include "graphics_api/descriptor/DescriptorHeap.hpp"
#include "graphics_api/command_recording/CommandList.hpp"
#include "graphics_api/command_recording/CommandQueue.hpp"
#include "LinearFrameAllocator.hpp"
#include "graphics_api/descriptor/ResourceView.hpp"
#include "scene/SceneRenderData.hpp"
#include "LinearAllocator.hpp"

namespace aZero
{
	namespace Rendering
	{
		// New wrappers and frame context
		struct FrameContext : public NonCopyable
		{
			aZero::LinearAllocator<> m_FrameLinearAllocator;
			RenderAPI::Buffer m_FrameDataBuffer;

			// Commandlist stuff
			RenderAPI::CommandList m_DirectCmdList;
			RenderAPI::CommandList m_CopyCmdList;
			RenderAPI::CommandList m_ComputeCmdList;
			//

			// Per-frame linear staging allocator that stores allocations and can record them at a later point
			LinearFrameAllocator m_FrameAllocator;

			template<typename T>
			void AddAllocation(const T& data, RenderAPI::Buffer& buffer, uint32_t dstOffset)
			{
				m_FrameAllocator.AddAllocation(&data, &buffer, dstOffset, sizeof(T));
			}

			// !note To be executed before rendering a scene since it might contain asset upload data and other things
			void RecordFrameAllocations(RenderAPI::CommandList& cmdList)
			{
				m_FrameAllocator.RecordAllocations(cmdList);
			}
			//

			uint64_t m_FrameCompleteSignal = 0; // Latest signal that is related to the context - Should be used to find when the frame context is available again

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
				return directQueue.WaitForSignal(m_FrameCompleteSignal, false);
			}

			FrameContext() = default;
			FrameContext(ID3D12DeviceX* device, RenderAPI::DescriptorHeap& resourceHeap, RenderAPI::ResourceRecycler& recycler, uint32_t maxInstances)
			{
				this->Init(device, resourceHeap, recycler, maxInstances);
			}

			FrameContext(FrameContext&&) noexcept {

			}
			FrameContext& operator=(FrameContext&&) noexcept = default;

			// TODO: Support dynamic resizing, or atleast easy resizing
			void Init(ID3D12DeviceX* device, RenderAPI::DescriptorHeap& resourceHeap, RenderAPI::ResourceRecycler& recycler, uint32_t maxInstances)
			{
				uint64_t frameBufferSize = static_cast<uint64_t>(1024 * 1024);
				m_FrameAllocator = LinearFrameAllocator(device, frameBufferSize, recycler);
				m_FrameDataBuffer = RenderAPI::Buffer(device, RenderAPI::Buffer::Desc(frameBufferSize, D3D12_HEAP_TYPE_UPLOAD), &recycler);
				m_FrameLinearAllocator = aZero::LinearAllocator<>(static_cast<std::byte*>(m_FrameDataBuffer.GetCPUAccessibleMemory()), frameBufferSize);

				m_DirectCmdList = RenderAPI::CommandList(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
				m_CopyCmdList = RenderAPI::CommandList(device, D3D12_COMMAND_LIST_TYPE_COPY);
				m_ComputeCmdList = RenderAPI::CommandList(device, D3D12_COMMAND_LIST_TYPE_COMPUTE);

#ifdef USE_DEBUG
				m_FrameDataBuffer.GetResource()->SetName(L"m_FrameDataBuffer");
#endif
			};

			// Used on engine frame beginning
			void Begin()
			{
				m_FrameAllocator.Reset();
				m_DirectCmdList.ClearCommandBuffer();
				m_CopyCmdList.ClearCommandBuffer();
				m_ComputeCmdList.ClearCommandBuffer();
				m_FrameLinearAllocator.Rewind();
			}
		};

	}
}