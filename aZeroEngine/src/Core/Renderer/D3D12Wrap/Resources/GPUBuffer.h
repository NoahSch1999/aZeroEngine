#pragma once
#include "GPUResource.h"

namespace aZero
{
	namespace D3D12
	{
		class GPUBuffer : public GPUResourceBase
		{
		private:
			int m_NumElements = -1;

		public:
			GPUBuffer() = default;

			GPUBuffer(
				int NumElements, 
				int BytesPerElement, 
				DXGI_FORMAT Format, 
				D3D12_HEAP_TYPE HeapType, 
				std::optional<ResourceRecycler*> OptResourceRecycler
			)
			{
				this->Init(NumElements, BytesPerElement, Format, HeapType, OptResourceRecycler);
			}

			/*GPUBuffer(GPUBuffer&& other) noexcept
			{
				m_NumElements = other.m_NumElements;
			}

			GPUBuffer& operator=(GPUBuffer&& other) noexcept
			{
				if (this != &other)
				{
					m_NumElements = other.m_NumElements;
				}

				return *this;
			}*/

			void Init(
				int NumElements, 
				int BytesPerElement, 
				DXGI_FORMAT Format, 
				D3D12_HEAP_TYPE HeapType, 
				std::optional<ResourceRecycler*> OptResourceRecycler
			)
			{
				m_NumElements = NumElements;

				D3D12_RESOURCE_DESC ResourceDesc;
				ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
				ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				ResourceDesc.MipLevels = 1;
				ResourceDesc.DepthOrArraySize = 1;
				ResourceDesc.Height = 1;
				ResourceDesc.Width = NumElements * BytesPerElement;
				ResourceDesc.SampleDesc.Count = 1;
				ResourceDesc.SampleDesc.Quality = 0;
				ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
				ResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
				ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				ResourceDesc.Format = Format;

				D3D12_HEAP_PROPERTIES HeapProperties;
				ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
				HeapProperties.Type = HeapType;

				D3D12_HEAP_FLAGS HeapFlags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;

				D3D12_RESOURCE_STATES InitialResourceState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;

				GPUResourceBase::Init(ResourceDesc, HeapProperties, HeapFlags, InitialResourceState, {}, OptResourceRecycler);
			}

			void Resize(int NumElements, int BytesPerElement, ID3D12GraphicsCommandList* CmdList)
			{
				int OldNumElements = m_NumElements;
				ID3D12Resource* OldResource = this->GetResource();

				m_NumElements = NumElements;

				D3D12_RESOURCE_DESC ResourceDesc = this->GetResource()->GetDesc();
				ResourceDesc.Width = NumElements * BytesPerElement;
				D3D12_HEAP_PROPERTIES HeapProperties;
				D3D12_HEAP_FLAGS HeapFlags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
				D3D12_RESOURCE_STATES InitialResourceState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
				HRESULT Hr = this->GetResource()->GetHeapProperties(&HeapProperties, &HeapFlags);
				if (FAILED(Hr))
				{
					throw;
				}

				GPUResourceBase::Init(ResourceDesc, HeapProperties, HeapFlags, InitialResourceState, {}, this->GetResourceRecyclerPtr());

				CmdList->CopyBufferRegion(this->GetResource(), 0, OldResource, 0, OldNumElements * BytesPerElement);
			}

			int GetNumElements() const { return m_NumElements; }
		};
	}
}