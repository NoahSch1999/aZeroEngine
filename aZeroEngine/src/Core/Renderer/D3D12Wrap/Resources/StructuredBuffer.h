#pragma once
#include "DefaultHeapBuffer.h"
#include "UploadHeapBuffer.h"

namespace aZero
{
	namespace D3D12
	{
		template<typename ElementType>
		class UploadHeapStructuredBuffer
		{
		private:
			UploadHeapBuffer m_Buffer;

		public:
			UploadHeapStructuredBuffer() = default;

			UploadHeapStructuredBuffer(int NumElements, DXGI_FORMAT Format, std::optional<ResourceRecycler*> OptResourceRecycler)
			{
				this->Init(NumElements, Format, OptResourceRecycler);
			}

			UploadHeapStructuredBuffer(UploadHeapStructuredBuffer&& other) noexcept
			{
				m_Buffer = std::move(other.m_Buffer);
			}

			UploadHeapStructuredBuffer& operator=(UploadHeapStructuredBuffer&& other) noexcept
			{
				if (this != &other)
				{
					m_Buffer = std::move(other.m_Buffer);
				}

				return *this;
			}

			void Init(int NumElements, DXGI_FORMAT Format, std::optional<ResourceRecycler*> OptResourceRecycler)
			{
				m_Buffer = std::move(UploadHeapBuffer(NumElements, sizeof(ElementType), Format, OptResourceRecycler));
			}

			template <typename BufferType>
			void Write(int DstElementIndex, int NumElements, const BufferType& Src, int SrcElementIndex, ID3D12GraphicsCommandList* CmdList)
			{
				m_Buffer.Write(DstElementIndex * sizeof(ElementType), Src, SrcElementIndex * sizeof(ElementType), NumElements * sizeof(ElementType), CmdList);
			}
			
			void Write(int DstElementIndex, int NumElements, void* Data)
			{
				m_Buffer.Write(DstElementIndex * sizeof(ElementType), Data, NumElements * sizeof(ElementType));
			}

			UploadHeapBuffer& GetInternalBuffer() { return m_Buffer; }

			int GetNumElements() const { return m_Buffer.GetNumElements(); }
		};

		template<typename ElementType>
		class DefaultHeapStructuredBuffer
		{
		private:
			DefaultHeapBuffer m_Buffer;

		public:
			DefaultHeapStructuredBuffer() = default;

			DefaultHeapStructuredBuffer(int NumElements, DXGI_FORMAT Format, std::optional<ResourceRecycler*> OptResourceRecycler)
			{
				this->Init(NumElements, Format, OptResourceRecycler);
			}

			DefaultHeapStructuredBuffer(DefaultHeapStructuredBuffer&& other) noexcept
			{
				m_Buffer = std::move(other.m_Buffer);
			}

			DefaultHeapStructuredBuffer& operator=(DefaultHeapStructuredBuffer&& other) noexcept
			{
				if (this != &other)
				{
					m_Buffer = std::move(other.m_Buffer);
				}

				return *this;
			}

			void Init(int NumElements, DXGI_FORMAT Format, std::optional<ResourceRecycler*> OptResourceRecycler)
			{
				m_Buffer = std::move(DefaultHeapBuffer(NumElements, sizeof(ElementType), Format, OptResourceRecycler));
			}

			template <typename BufferType>
			void Write(int DstElementIndex, int NumElements, const BufferType& Src, int SrcElementIndex, ID3D12GraphicsCommandList* CmdList)
			{
				m_Buffer.Write(DstElementIndex * sizeof(ElementType), Src, SrcElementIndex * sizeof(ElementType), NumElements * sizeof(ElementType), CmdList);
			}

			void Write(int DstElementIndex, int NumElements, void* Data, UploadHeapBuffer& SrcIntermediate, int SrcElementIndex, ID3D12GraphicsCommandList* CmdList)
			{
				m_Buffer.Write(DstElementIndex * sizeof(ElementType), Data, SrcIntermediate, SrcElementIndex * sizeof(ElementType), NumElements * sizeof(ElementType), CmdList);
			}

			DefaultHeapBuffer& GetInternalBuffer() { return m_Buffer; }

			int GetNumElements() const { return m_Buffer.GetNumElements(); }
		};
	}
}