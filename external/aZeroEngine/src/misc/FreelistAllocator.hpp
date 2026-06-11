#pragma once
#include <list>
#include <algorithm>

namespace aZero
{
    class FreelistAllocator
    {
        struct Chunk
        {
            bool IsFree;
            size_t Offset;
            size_t Size;
        };
    public:
        struct Allocation
        {
            size_t Offset;
            size_t Size;
        };

        FreelistAllocator() = default;
        explicit FreelistAllocator(size_t size)
        {
            m_Chunks.emplace_back(true, 0, size);
        }

        [[nodiscard]] Allocation Allocate(size_t size)
        {
            auto it = std::find_if(m_Chunks.begin(), m_Chunks.end(),
                [&size](const Chunk& chunk) {
                    return chunk.IsFree && chunk.Size >= size;
                });

            if (it == m_Chunks.end() || size == 0)
            {
                throw std::bad_alloc();
            }

            const size_t chunkOffset = it->Offset;
            const size_t chunkSize = it->Size;

            it->Size = size;
            it->IsFree = false;
            if (chunkSize != size)
            {
                m_Chunks.insert(std::next(it), { .IsFree = true, .Offset = chunkOffset + size, .Size = chunkSize - size });
            }

            return { .Offset = chunkOffset, .Size = size };
        }

        void Free(const Allocation& allocation)
        {
            for (auto iter = m_Chunks.begin(); iter != m_Chunks.end(); iter++)
            {
                if (iter->Offset == allocation.Offset)
                {
                    auto prev = (iter == m_Chunks.begin())
                        ? m_Chunks.end()
                        : std::prev(iter);

                    auto next = std::next(iter);

                    const bool mergeWithPrev = prev != m_Chunks.end() && prev->IsFree;
                    const bool mergeWithNext = next != m_Chunks.end() && next->IsFree;

                    iter->IsFree = true;

                    if (mergeWithPrev && mergeWithNext)
                    {
                        prev->Size += iter->Size + next->Size;
                        m_Chunks.erase(iter);
                        m_Chunks.erase(next);
                    }
                    else if (mergeWithPrev)
                    {
                        prev->Size += iter->Size;
                        m_Chunks.erase(iter);
                    }
                    else if (mergeWithNext)
                    {
                        next->Offset = iter->Offset;
                        next->Size += iter->Size;
                        m_Chunks.erase(iter);
                    }

                    return;
                }
            }
        }

        [[nodiscard]] size_t GetLargestFreeSize() const
        {
            size_t largestSize = 0;
            for (const auto& chunk : m_Chunks)
            {
                if (chunk.IsFree && chunk.Size > largestSize)
                {
                    largestSize = chunk.Size;
                }
            }
            return largestSize;
        }

        [[nodiscard]] bool IsFull() const
        {
            return std::all_of(m_Chunks.begin(), m_Chunks.end(), [](const Chunk& chunk) {return !chunk.IsFree; });
        }

        [[nodiscard]] bool CanAllocate(size_t size) const
        {
            if (size == 0)
            {
                return false;
            }

            auto it = std::find_if(m_Chunks.begin(), m_Chunks.end(),
                [&size](const Chunk& chunk) {
                    return chunk.IsFree && chunk.Size >= size;
                });

            return it != m_Chunks.end();
        }

    private:
        std::list<Chunk> m_Chunks;
    };
}