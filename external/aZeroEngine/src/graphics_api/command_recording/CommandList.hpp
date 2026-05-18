#pragma once
#include <optional>
#include "misc/NonCopyable.hpp"
#include "graphics_api/D3D12Include.hpp"
#include "pipeline/RenderPass.hpp"

namespace aZero
{
	namespace RenderAPI
	{
		class DescriptorHeap;
		class Descriptor;

		class CommandList : public NonCopyable
		{
		public:
			CommandList() = default;
			CommandList(ID3D12DeviceX* device, D3D12_COMMAND_LIST_TYPE type);

			CommandList(CommandList&& other) noexcept;
			CommandList& operator=(CommandList&& other) noexcept;

			ID3D12GraphicsCommandListX* operator->() { return m_CommandList.Get(); }
			const ID3D12GraphicsCommandListX* operator->() const { return m_CommandList.Get(); }

			void ClearCommandBuffer();

			void StartRecording();
			void StopRecording();

			D3D12_COMMAND_LIST_TYPE GetType() const { return m_CommandList->GetType(); }
			ID3D12GraphicsCommandListX* Get() const { return m_CommandList.Get(); }
			bool IsInitiated() const { return m_Allocator != nullptr; }
			bool IsRecording() const { return m_IsRecording; }

			void SetDescriptorHeaps(const RenderAPI::DescriptorHeap& resourceHeap, const RenderAPI::DescriptorHeap& samplerHeap);

			void OMSetRenderTargets(const std::vector<std::reference_wrapper<RenderAPI::Descriptor>>& renderTargets, const std::optional<std::reference_wrapper<RenderAPI::Descriptor>>& depthStencilTarget);

			void SetGraphicsRootShaderResourceViewSafe(uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS address)
			{
				if (rootParameterIndex != std::numeric_limits<uint32_t>::max())
				{
					m_CommandList->SetGraphicsRootShaderResourceView(rootParameterIndex, address);
				}
			}

			void SetGraphicsRootUnorderedAccessViewSafe(uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS address)
			{
				if (rootParameterIndex != std::numeric_limits<uint32_t>::max())
				{
					m_CommandList->SetGraphicsRootUnorderedAccessView(rootParameterIndex, address);
				}
			}

			void SetGraphicsRoot32BitConstantsSafe(uint32_t rootParameterIndex, uint32_t num32BitValuesToSet, const void* pSrcData, uint32_t destOffsetIn32BitValues)
			{
				if (rootParameterIndex != std::numeric_limits<uint32_t>::max())
				{
					m_CommandList->SetGraphicsRoot32BitConstants(rootParameterIndex, num32BitValuesToSet, pSrcData, destOffsetIn32BitValues);
				}
			}

			void SetComputeRootShaderResourceViewSafe(uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS address)
			{
				if (rootParameterIndex != std::numeric_limits<uint32_t>::max())
				{
					m_CommandList->SetComputeRootShaderResourceView(rootParameterIndex, address);
				}
			}

			void SetComputeRootUnorderedAccessViewSafe(uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS address)
			{
				if (rootParameterIndex != std::numeric_limits<uint32_t>::max())
				{
					m_CommandList->SetComputeRootUnorderedAccessView(rootParameterIndex, address);
				}
			}

			void SetComputeRoot32BitConstantsSafe(uint32_t rootParameterIndex, uint32_t num32BitValuesToSet, const void* pSrcData, uint32_t destOffsetIn32BitValues)
			{
				if (rootParameterIndex != std::numeric_limits<uint32_t>::max())
				{
					m_CommandList->SetComputeRoot32BitConstants(rootParameterIndex, num32BitValuesToSet, pSrcData, destOffsetIn32BitValues);
				}
			}

			// NEW
			void SetGraphicsRootShaderResourceViewSafe(std::optional<std::reference_wrapper<aZero::Pipeline::BufferBinding>> bufferBinding, D3D12_GPU_VIRTUAL_ADDRESS address)
			{
				if (bufferBinding.has_value())
				{
					m_CommandList->SetGraphicsRootShaderResourceView(bufferBinding.value().get().GetRootIndex(), address);
				}
			}

			void SetGraphicsRootUnorderedAccessViewSafe(std::optional<std::reference_wrapper<aZero::Pipeline::BufferBinding>> bufferBinding, D3D12_GPU_VIRTUAL_ADDRESS address)
			{
				if (bufferBinding.has_value())
				{
					m_CommandList->SetGraphicsRootUnorderedAccessView(bufferBinding.value().get().GetRootIndex(), address);
				}
			}

			void SetGraphicsConstantBufferViewSafe(std::optional<std::reference_wrapper<aZero::Pipeline::BufferBinding>> bufferBinding, D3D12_GPU_VIRTUAL_ADDRESS address)
			{
				if (bufferBinding.has_value())
				{
					m_CommandList->SetGraphicsRootConstantBufferView(bufferBinding.value().get().GetRootIndex(), address);
				}
			}

			void SetGraphicsRoot32BitConstantsSafe(std::optional<std::reference_wrapper<aZero::Pipeline::ConstantBinding>> constantBinding, const void* pSrcData, uint32_t destOffsetIn32BitValues)
			{
				if (constantBinding.has_value())
				{
					m_CommandList->SetGraphicsRoot32BitConstants(constantBinding.value().get().GetRootIndex(), constantBinding.value().get().GetNumConstants(), pSrcData, destOffsetIn32BitValues);
				}
			}

			void SetComputeRootShaderResourceViewSafe(std::optional<std::reference_wrapper<aZero::Pipeline::BufferBinding>> bufferBinding, D3D12_GPU_VIRTUAL_ADDRESS address)
			{
				if (bufferBinding.has_value())
				{
					m_CommandList->SetComputeRootShaderResourceView(bufferBinding.value().get().GetRootIndex(), address);
				}
			}

			void SetComputeRootUnorderedAccessViewSafe(std::optional<std::reference_wrapper<aZero::Pipeline::BufferBinding>> bufferBinding, D3D12_GPU_VIRTUAL_ADDRESS address)
			{
				if (bufferBinding.has_value())
				{
					m_CommandList->SetComputeRootUnorderedAccessView(bufferBinding.value().get().GetRootIndex(), address);
				}
			}

			void SetComputeConstantBufferViewSafe(std::optional<std::reference_wrapper<aZero::Pipeline::BufferBinding>> bufferBinding, D3D12_GPU_VIRTUAL_ADDRESS address)
			{
				if (bufferBinding.has_value())
				{
					m_CommandList->SetComputeRootConstantBufferView(bufferBinding.value().get().GetRootIndex(), address);
				}
			}

			void SetComputeRoot32BitConstantsSafe(std::optional<std::reference_wrapper<aZero::Pipeline::ConstantBinding>> constantBinding, const void* pSrcData, uint32_t destOffsetIn32BitValues)
			{
				if (constantBinding.has_value())
				{
					m_CommandList->SetComputeRoot32BitConstants(constantBinding.value().get().GetRootIndex(), constantBinding.value().get().GetNumConstants(), pSrcData, destOffsetIn32BitValues);
				}
			}

		private:
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_Allocator = nullptr;
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandListX> m_CommandList = nullptr;
			D3D12_COMMAND_LIST_TYPE m_Type = D3D12_COMMAND_LIST_TYPE_NONE;
			bool m_IsRecording = true;
		};
	}
}