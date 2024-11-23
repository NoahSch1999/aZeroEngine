#pragma once
#include "DefaultHeapBuffer.h"
#include "FrameAllocator.h"
#include <algorithm>
#include <queue>

namespace aZero
{
	namespace D3D12
	{
		template<typename ElementType>
		class PackedGPULookupBuffer
		{
		private:
			std::queue<UINT> m_FreeIndices;
			std::vector<UINT> m_IDtoElementIndexMap;
			std::unordered_map<UINT, UINT> m_ElementIndexToIDMap;
			D3D12::DefaultHeapBuffer m_Buffer;
			UINT m_NextIndex = 0;
			UINT m_MaxAssignedElements = 0;

			void ResizeGPUBuffer(ID3D12GraphicsCommandList* CmdList, UINT MinAssignedElements)
			{
				
				UINT NewMaxNumElements = m_MaxAssignedElements * 2;
				NewMaxNumElements = max(NewMaxNumElements, MinAssignedElements);
				
				// If the new maximum is more than the currently supported entries -> Reduce realloc size to m_IDtoElementIndexMap.size()
				if (NewMaxNumElements > m_IDtoElementIndexMap.size())
				{
					const UINT DifferenceInElements = NewMaxNumElements - m_IDtoElementIndexMap.size();
					NewMaxNumElements -= DifferenceInElements;
				}

				m_Buffer.Resize(NewMaxNumElements, sizeof(ElementType), CmdList);
				m_MaxAssignedElements = NewMaxNumElements;
			}

		public:
			PackedGPULookupBuffer() = default;

			PackedGPULookupBuffer(int StartElements, int MaxElements)
			{
				this->Init(StartElements, MaxElements);
			}

			void Init(int StartElements, int MaxElements)
			{
				m_IDtoElementIndexMap.resize(MaxElements, UINT_MAX);
				m_Buffer.Init(StartElements, sizeof(ElementType), DXGI_FORMAT_UNKNOWN, &gResourceRecycler);
				m_MaxAssignedElements = StartElements;
			}

			void Register(UINT Id)
			{
				if (Id >= m_IDtoElementIndexMap.size())
				{
					throw;
				}

				if (m_IDtoElementIndexMap.at(Id) != UINT_MAX)
				{
					return;
				}

				if (m_FreeIndices.empty())
				{
					m_IDtoElementIndexMap.at(Id) = m_NextIndex;
					m_ElementIndexToIDMap.emplace(m_NextIndex, Id);
					m_NextIndex++;
				}
				else
				{
					const UINT NextEmptyIndex = m_FreeIndices.front();
					m_FreeIndices.pop();
					m_IDtoElementIndexMap.at(Id) = NextEmptyIndex;
					m_ElementIndexToIDMap.emplace(NextEmptyIndex, Id);
				}
			}

			// If update has been recorded for copy to X, but ::Remove has changed the address of X, the copy will copy to incorrect address
			// Solved by executing update commands before remove commands
			void Remove(UINT Id)
			{
				if (Id >= m_IDtoElementIndexMap.size())
				{
					throw;
				}

				if (m_IDtoElementIndexMap.at(Id) == UINT_MAX)
				{
					return;
				}

				const UINT RemovedElementIndex = m_IDtoElementIndexMap.at(Id);
				m_FreeIndices.push(RemovedElementIndex);
				m_IDtoElementIndexMap.at(Id) = UINT_MAX;
				m_ElementIndexToIDMap.erase(RemovedElementIndex);
			}

			void Update(UINT Id, void* Data, FrameAllocator& Allocator, UINT ElementDataOffset = 0, UINT NumBytes = sizeof(ElementType));
			

			bool IsRegistered(UINT Id) const
			{
				if (Id >= m_IDtoElementIndexMap.size())
				{
					return false;
				}

				return m_IDtoElementIndexMap.at(Id) != UINT_MAX;
			}
			
			void ResizeMaxEntries(UINT AdditionalEntries)
			{
				std::vector<UINT> NewIDToElementIndexMap = m_IDtoElementIndexMap;
				NewIDToElementIndexMap.resize(NewIDToElementIndexMap.size() + AdditionalEntries, UINT_MAX);
			}

			ID3D12Resource* GetResource() { return m_Buffer.GetResource(); }
			UINT GetNumElements() { return m_MaxAssignedElements; }
		};

		template<typename ElementType>
		inline void PackedGPULookupBuffer<ElementType>::Update(UINT Id, void* Data, FrameAllocator& Allocator, UINT ElementDataOffset, UINT NumBytes)
		{
			if (m_IDtoElementIndexMap.at(Id) == UINT_MAX)
			{
				return;
			}

			if (m_NextIndex > m_MaxAssignedElements)
			{
				this->ResizeGPUBuffer(Allocator.GetCommandList(), m_NextIndex);
			}

			const UINT AllocGPUDst = m_IDtoElementIndexMap.at(Id);

			Allocator.AddAllocation(
				Data,
				&this->m_Buffer,
				AllocGPUDst * sizeof(ElementType) + ElementDataOffset,
				NumBytes
			);

		}
	}
}