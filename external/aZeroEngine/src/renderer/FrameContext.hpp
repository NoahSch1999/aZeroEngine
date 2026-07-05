#pragma once
#include "render_api/resource/buffer/Buffer.hpp"
#include "render_api/resource/texture/Texture2D.hpp"
#include "render_api/descriptor/DescriptorHeap.hpp"
#include "render_api/command_recording/CommandList.hpp"
#include "render_api/command_recording/CommandQueue.hpp"
#include "FrameStagingAllocator.hpp"
#include "render_api/descriptor/ResourceView.hpp"
#include <LinearAllocator.hpp>

namespace aZero
{
	namespace Rendering
	{
		class FrameContext : public NonCopyable
		{
		public:
			FrameContext() = default;

			FrameContext(ID3D12DeviceX* device, RenderAPI::DescriptorHeap& resourceHeap, RenderAPI::ResourceRecycler& recycler, uint32_t maxInstances)
			{
				// TODO: Support dynamic resizing, or atleast easy resizing
				uint64_t frameBufferSize = static_cast<uint64_t>(1024 * 1024);
				m_FrameStagingAllocator = FrameStagingAllocator(device, frameBufferSize, recycler);
				m_FrameUploadBuffer = RenderAPI::Buffer(device, RenderAPI::Buffer::Desc(frameBufferSize, D3D12_HEAP_TYPE_UPLOAD), &recycler);
				m_FrameUploadAllocator = aZero::LinearAllocator<>(static_cast<std::byte*>(m_FrameUploadBuffer.GetCPUAccessibleMemory()), frameBufferSize);

				m_DirectCmdList = RenderAPI::CommandList(device, D3D12_COMMAND_LIST_TYPE_DIRECT);

#ifdef USE_DEBUG
				m_FrameUploadBuffer.GetResource()->SetName(L"m_FrameDataBuffer");
#endif
			}

			FrameContext(FrameContext&&) noexcept = default;
			FrameContext& operator=(FrameContext&&) noexcept = default;

			template<typename T>
			void AddAllocation(const T& data, RenderAPI::Buffer& buffer, uint32_t dstOffset)
			{
				m_FrameStagingAllocator.AddAllocation(&data, &buffer, dstOffset, sizeof(T));
			}

			// !note To be executed before rendering a scene since it might contain asset upload data and other things
			void RecordFrameAllocations(RenderAPI::CommandList& cmdList)
			{
				m_FrameStagingAllocator.RecordAllocations(cmdList);
			}

			void SetLatestSignal(uint64_t signal)
			{
				m_FrameCompleteSignal = signal;
			}

			bool IsReady(RenderAPI::CommandQueue& directQueue)
			{
				return directQueue.WaitForSignal(m_FrameCompleteSignal, false);
			}

			void Begin()
			{
				m_FrameStagingAllocator.Reset();
				m_DirectCmdList.ClearCommandBuffer();
				m_FrameUploadAllocator.Rewind();
			}

			RenderAPI::CommandList& GetCommandList() { return m_DirectCmdList; }
			FrameStagingAllocator& GetFrameStagingAllocator() { return m_FrameStagingAllocator; }
			aZero::LinearAllocator<>& GetFrameUploadAllocator() { return m_FrameUploadAllocator; }
			RenderAPI::Buffer& GetFrameUploadBuffer() { return m_FrameUploadBuffer; }

		private:
			// Per-frame linear staging allocator that stores allocations and can record them at a later point
			FrameStagingAllocator m_FrameStagingAllocator;

			uint64_t m_FrameCompleteSignal = 0; // Latest signal that is related to the context - Should be used to find when the frame context is available again

			aZero::LinearAllocator<> m_FrameUploadAllocator;
			RenderAPI::Buffer m_FrameUploadBuffer;

			RenderAPI::CommandList m_DirectCmdList;
		};

	}
}