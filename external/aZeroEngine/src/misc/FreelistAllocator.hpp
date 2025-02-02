#pragma once
#include <list>
#include "misc/NonCopyable.hpp"

namespace aZero
{
	namespace DS
	{
		class FreelistAllocator
		{
		private:
			struct AllocationNode
			{
				uint32_t m_Offset;
				uint32_t m_NumBytes;
			};

			uint32_t m_CurrentSize;
			std::list<AllocationNode> m_FreeNodes;

			// TODO: Optimize
			void InsertNode(const AllocationNode& InNode)
			{
				std::list<AllocationNode>::iterator it;
				for (it = m_FreeNodes.begin(); it != m_FreeNodes.end(); it++)
				{
					// If node is right after current -> add it to current
					if (InNode.m_Offset == it->m_Offset + it->m_NumBytes)
					{
						it->m_NumBytes += InNode.m_NumBytes;

						std::list<AllocationNode>::iterator next = std::next(it);
						if (next != m_FreeNodes.end())
						{
							if (it->m_Offset + it->m_NumBytes == next->m_Offset)
							{
								it->m_NumBytes += next->m_NumBytes;
								m_FreeNodes.erase(next);
							}
						}

						return;
					}
					else if (InNode.m_Offset + InNode.m_NumBytes == it->m_Offset)
					{
						it->m_NumBytes += InNode.m_NumBytes;
						it->m_Offset = InNode.m_Offset;
						return;
					}
				}

				m_FreeNodes.push_back(InNode);
			}
		public:

			class AllocationHandle : public NonCopyable
			{
				friend FreelistAllocator;
			private:
				AllocationNode m_Node;
				FreelistAllocator* m_Owner = nullptr;

				AllocationHandle(FreelistAllocator& Owner, AllocationNode InNode)
					:m_Owner(&Owner), m_Node(InNode)
				{

				}
			public:
				AllocationHandle() = default;

				~AllocationHandle()
				{
					if (this->IsValid())
					{
						m_Owner->FreeAllocation(*this);
					}
				}

				AllocationHandle(AllocationHandle&& Other) noexcept
				{
					m_Node = Other.m_Node;
					m_Owner = Other.m_Owner;

					Other.m_Owner = nullptr;
				}

				AllocationHandle& operator=(AllocationHandle&& Other) noexcept
				{
					if (this != &Other)
					{
						m_Node = Other.m_Node;
						m_Owner = Other.m_Owner;

						Other.m_Owner = nullptr;
					}

					return *this;
				}

				uint32_t GetStartOffset() const { return m_Node.m_Offset; }
				uint32_t GetNumBytes() const { return m_Node.m_NumBytes; }

				bool IsValid() const { return m_Owner != nullptr; }
			};

			FreelistAllocator() = default;

			FreelistAllocator(uint32_t SizeBytes)
			{
				this->Init(SizeBytes);
			}

			void Init(uint32_t SizeBytes)
			{
				m_CurrentSize = SizeBytes;

				AllocationNode InitNode;
				InitNode.m_Offset = 0;
				InitNode.m_NumBytes = SizeBytes;
				m_FreeNodes.push_back(InitNode);
			}

			bool Allocate(AllocationHandle& OutHandle, unsigned int NumBytes)
			{
				std::list<AllocationNode>::iterator it;
				for (it = m_FreeNodes.begin(); it != m_FreeNodes.end(); it++)
				{
					if (it->m_NumBytes > NumBytes)
					{
						this->FreeAllocation(OutHandle);

						AllocationNode NewNode = *it;
						const unsigned int LeftoverBytes = NewNode.m_NumBytes - NumBytes;
						NewNode.m_NumBytes -= LeftoverBytes;
						NewNode.m_Offset = it->m_Offset;

						OutHandle = std::move(AllocationHandle(*this, NewNode));

						AllocationNode LeftoverNode;
						LeftoverNode.m_NumBytes = LeftoverBytes;
						LeftoverNode.m_Offset = NewNode.m_Offset + NewNode.m_NumBytes;

						m_FreeNodes.erase(it);
						this->InsertNode(LeftoverNode);
						return true;
					}
				}

				return false;
			}

			void FreeAllocation(AllocationHandle& InHandle)
			{
				if (InHandle.IsValid())
				{
					this->InsertNode(InHandle.m_Node);
					InHandle.m_Owner = nullptr;
				}
			}
		};
	}
}