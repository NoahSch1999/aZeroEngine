#pragma once

#include <cstdint>
#include <stdexcept>

namespace aZero
{
	/// <summary>
	/// A thin wrapper-class that enables memory-aligned linear allocations on an underlying memory pool.
	/// All offsets and sizes are defined in bytes.
	/// </summary>
	/// <typeparam name="AlignmentBytes">How many bytes allocations should be aligned with.</typeparam>
	template<size_t AlignmentBytes = sizeof(int32_t)>
	class LinearAllocator
	{
	public:
		/// <summary>
		/// Information about an allocation.
		/// </summary>
		struct Allocation
		{
			/// <summary>
			/// Offset into the memory pool in bytes.
			/// </summary>
			size_t Offset;

			/// <summary>
			/// Size in bytes.
			/// </summary>
			size_t Size;
		};

		LinearAllocator() = default;

		/// <summary>
		/// Initializes with the input memory pool and sets the current max size.
		/// </summary>
		/// <param name="memoryPool">A pointer to the memory pool.</param>
		/// <param name="size">Max size of the allocator.</param>
		LinearAllocator(void* memoryPool, size_t size)
		{
			this->Init(memoryPool, size);
		}

		LinearAllocator(const LinearAllocator&) = delete;
		LinearAllocator& operator=(const LinearAllocator&) = delete;

		LinearAllocator(LinearAllocator&& other) noexcept
		{
			this->Move(other);
		}

		LinearAllocator& operator=(LinearAllocator&& other) noexcept
		{
			if (this != &other)
			{
				this->Move(other);
			}
			return *this;
		}

		/// <summary>
		/// Initializes with the input memory pool and sets the current max size.
		/// </summary>
		/// <param name="memoryPool">A pointer to the memory pool.</param>
		/// <param name="size">Max size of the allocator.</param>
		void Init(void* memoryPool, size_t size)
		{
			m_MemoryPool = memoryPool;
			m_Size = size;
			m_Offset = 0;
		}

		/// <summary>
		/// Allocates a portion of the memory pool and returns a handle for it.
		/// </summary>
		/// <param name="size">Allocation size.</param>
		/// <returns>A handle describing the allocation.</returns>
		Allocation Allocate(size_t size)
		{
			void* offsetPtr = (std::byte*)m_MemoryPool + m_Offset;
			size_t spaceLeft = m_Size - m_Offset;
			if (!std::align(m_AlignmentBytes, size, offsetPtr, spaceLeft))
			{
				throw std::out_of_range("Memorypool is full");
			}

			const size_t allocOffset = (std::byte*)offsetPtr - (std::byte*)m_MemoryPool;
			m_Offset = allocOffset + size;
			return { .Offset = allocOffset, .Size = size };
		}

		/// <summary>
		/// Writes data to the part of the memory pool that the Allocation handle describes.
		/// NOTE: The size described in the Allocation handle shouldn't exceed the size of the data.
		/// </summary>
		/// <param name="allocation">Describes where the data should be written inside the memory pool.</param>
		/// <param name="data">Address of the data that should be written.</param>
		void Write(const Allocation& allocation, void* data)
		{
			memcpy((std::byte*)m_MemoryPool + allocation.Offset, data, allocation.Size);
		}

		/// <summary>
		/// Allocates a portion of the memory pool, writes the data to it, and returns a handle for it.
		/// </summary>
		/// <param name="data">Address of the data that should be allocated and written.</param>
		/// <param name="size">Allocation size.</param>
		/// <returns></returns>
		Allocation Append(void* data, size_t size)
		{
			const Allocation allocation = this->Allocate(size);
			this->Write(allocation, data);
			return allocation;
		}

		/// <summary>
		/// Gets a pointer to the data referenced by the Allocation handle.
		/// </summary>
		/// <param name="allocation">Allocation handle defining the data offset into the memory pool.</param>
		/// <returns>Address to the allocation.</returns>
		void* Get(const Allocation& allocation)
		{
			return (std::byte*)m_MemoryPool + allocation.Offset;
		}

		/// <summary>
		/// Resets the allocator to the beginning again.
		/// </summary>
		void Reset()
		{
			m_Offset = 0;
		}

		/// <summary>
		/// Gets the current offset into the memory pool.
		/// </summary>
		/// <returns>Current offset into the memory pool.</returns>
		size_t Offset() const { return m_Offset; }

		/// <summary>
		/// Gets the max size of the memory pool.
		/// </summary>
		/// <returns>Max size of the memory pool</returns>
		size_t Size() const { return m_Size; }

	private:
		void Move(LinearAllocator& other)
		{
			m_AlignmentBytes = other.m_AlignmentBytes;
			m_MemoryPool = other.m_MemoryPool;
			m_Offset = other.m_Offset;
			m_Size = other.m_Size;

			other.m_AlignmentBytes = 0;
			other.m_MemoryPool = nullptr;
			other.m_Offset = 0;
			other.m_Size = 0;
		}

		/// <summary>
		/// How many bytes allocations should be aligned with.
		/// </summary>
		size_t m_AlignmentBytes = AlignmentBytes;

		/// <summary>
		/// Pointer to the memory pool.
		/// </summary>
		void* m_MemoryPool = nullptr;

		/// <summary>
		/// Current offset into the memory pool.
		/// </summary>
		size_t m_Offset;

		/// <summary>
		/// Max size of the memory pool. Should never exceed the memoryreferenced by m_MemoryPool.
		/// </summary>
		size_t m_Size;
	};
}