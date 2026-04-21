#pragma once
#include "graphics_api/resource/buffer/Buffer.hpp"
#include "graphics_api/resource/texture/Texture2D.hpp"
#include "graphics_api/descriptor/DescriptorHeap.hpp"
#include "graphics_api/command_recording/CommandList.hpp"
#include "graphics_api/command_recording/CommandQueue.hpp"
#include "LinearFrameAllocator.hpp"
#include "graphics_api/descriptor/ResourceView.hpp"
#include "scene/SceneRenderData.hpp"

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
			RenderAPI::Buffer m_StaticMeshBuffer;
			RenderAPI::ShaderResourceView m_StaticMeshDescriptor;

			RenderAPI::Buffer m_PointLightBuffer;
			RenderAPI::ShaderResourceView m_PointLightDescriptor;

			RenderAPI::Buffer m_SpotLightBuffer;
			RenderAPI::ShaderResourceView m_SpotLightDescriptor;

			RenderAPI::Buffer m_DirectionalLightBuffer;
			RenderAPI::ShaderResourceView m_DirectionalLightDescriptor;

			RenderAPI::Buffer m_CameraBuffer;
			RenderAPI::ShaderResourceView m_CameraDescriptor;
			//

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
			FrameContext(ID3D12DeviceX* device, RenderAPI::DescriptorHeap& resourceHeap, RenderAPI::ResourceRecycler& recycler)
			{
				this->Init(device, resourceHeap, recycler);
			}

			FrameContext(FrameContext&&) noexcept {

			}
			FrameContext& operator=(FrameContext&&) noexcept = default;

			// TODO: Support dynamic resizing, or atleast easy resizing
			void Init(ID3D12DeviceX* device, RenderAPI::DescriptorHeap& resourceHeap, RenderAPI::ResourceRecycler& recycler)
			{
				const uint32_t frameBufferSize = 1000000;
				m_FrameAllocator = LinearFrameAllocator(device, frameBufferSize, recycler);

				RenderAPI::Buffer::Desc primitiveBufferDesc(0, D3D12_HEAP_TYPE_UPLOAD);
				primitiveBufferDesc.NumBytes = sizeof(Scene::RenderData::StaticMesh) * 1000;
				m_StaticMeshBuffer = RenderAPI::Buffer(device, primitiveBufferDesc, &recycler);

				primitiveBufferDesc.NumBytes = sizeof(Scene::RenderData::PointLight) * 1000;
				m_PointLightBuffer = RenderAPI::Buffer(device, primitiveBufferDesc, &recycler);
				primitiveBufferDesc.NumBytes = sizeof(Scene::RenderData::SpotLight) * 1000;
				m_SpotLightBuffer = RenderAPI::Buffer(device, primitiveBufferDesc, &recycler);
				primitiveBufferDesc.NumBytes = sizeof(Scene::RenderData::DirectionalLight) * 100;
				m_DirectionalLightBuffer = RenderAPI::Buffer(device, primitiveBufferDesc, &recycler);

				primitiveBufferDesc.NumBytes = sizeof(Scene::RenderData::Camera::GPUVersion) * 100;
				m_CameraBuffer = RenderAPI::Buffer(device, primitiveBufferDesc, &recycler);

				m_DirectCmdList = RenderAPI::CommandList(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
				m_CopyCmdList = RenderAPI::CommandList(device, D3D12_COMMAND_LIST_TYPE_COPY);
				m_ComputeCmdList = RenderAPI::CommandList(device, D3D12_COMMAND_LIST_TYPE_COMPUTE);

				m_StaticMeshDescriptor = RenderAPI::ShaderResourceView(device, resourceHeap, m_StaticMeshBuffer, 1000, sizeof(Scene::RenderData::StaticMesh), 0);

				m_PointLightDescriptor = RenderAPI::ShaderResourceView(device, resourceHeap, m_PointLightBuffer, 1000, sizeof(Scene::RenderData::PointLight), 0);
				m_SpotLightDescriptor = RenderAPI::ShaderResourceView(device, resourceHeap, m_SpotLightBuffer, 1000, sizeof(Scene::RenderData::SpotLight), 0);
				m_DirectionalLightDescriptor = RenderAPI::ShaderResourceView(device, resourceHeap, m_DirectionalLightBuffer, 100, sizeof(Scene::RenderData::DirectionalLight), 0);

				m_CameraDescriptor = RenderAPI::ShaderResourceView(device, resourceHeap, m_CameraBuffer, 100, sizeof(Scene::RenderData::Camera::GPUVersion), 0);

#ifdef USE_DEBUG
				m_StaticMeshBuffer.GetResource()->SetName(L"m_StaticMeshBuffer");

				m_PointLightBuffer.GetResource()->SetName(L"m_PointLightBuffer");
				m_SpotLightBuffer.GetResource()->SetName(L"m_SpotLightBuffer");
				m_DirectionalLightBuffer.GetResource()->SetName(L"m_DirectionalLightBuffer");

				m_CameraBuffer.GetResource()->SetName(L"m_CameraBuffer");
#endif
			};

			// Used on engine frame beginning
			void Begin()
			{
				m_FrameAllocator.Reset();
				m_DirectCmdList.ClearCommandBuffer();
				m_CopyCmdList.ClearCommandBuffer();
				m_ComputeCmdList.ClearCommandBuffer();
			}
		};

	}
}