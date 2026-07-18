#pragma once
#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace aZero
{
    /**
     * @brief A wrapper that allows linear suballocations into a designated memory pool. All allocations have to be for classes that are trivially copiable.
     * @tparam AlignmentType The desired type to align for. Defaulted to int32_t (4 bytes).
     */
    template<typename AlignmentType = int32_t>
    class LinearAllocator
    {
    public:
        /**
         * @brief The desired alignment in bytes
         */
        static constexpr size_t Alignment = sizeof(AlignmentType);

        /**
         * @brief A handle to an allocation. Can be used to write and read to the allocated memory.
         * @tparam T The type of the class that was allocated for.
         */
        template<typename T>
        class Handle
        {
            friend LinearAllocator;
            std::byte* m_OffsetPtr = nullptr;

            Handle(std::byte* offsetPtr)
                :m_OffsetPtr(offsetPtr) { }
        public:
            Handle() = default;

            /**
              * @brief Copies data to the allocated address.
              * @param data The data to copy.
              */
            void Write(const T& data) const { memcpy(m_OffsetPtr, &data, sizeof(T)); }

            T* operator->() { return reinterpret_cast<T*>(m_OffsetPtr); }
            T& operator*() { return *reinterpret_cast<T*>(m_OffsetPtr); }
            T& operator*() const { return *reinterpret_cast<T*>(m_OffsetPtr); }
        };

        LinearAllocator() = default;

        /**
         * @brief Main constructor for initialization.
         * @param memoryPool A pointer to an allocated memory pool that will be suballocated into by the allocator.
         * @param capacity The capacity (in bytes) of the allocated memory pool that the pointer references. NOTE: Can lead to UB if it's larger than the actual allocated memory pool.
         */
        explicit LinearAllocator(std::byte* memoryPool, size_t capacity)
            :m_MemoryPool(memoryPool), m_OffsetPtr(memoryPool), m_Capacity(capacity) { }

        LinearAllocator(LinearAllocator&) = delete;
        LinearAllocator& operator=(LinearAllocator&) = delete;

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

        /**
         * @brief Returns a handle to the allocation with the size of the template parameter class. NOTE: All allocations have to be for classes that are trivially copyable.
         * @tparam T The class to allocate for.
         * @return A handle to the allocation.
         */
        template<typename T>
        [[nodiscard]] Handle<T> Allocate()
        {
            static_assert(std::is_trivially_copyable_v<T>);

            size_t freeBytes = m_Capacity - static_cast<size_t>(m_OffsetPtr - m_MemoryPool);
            void* voidOffsetPtr = static_cast<void*>(m_OffsetPtr);
            std::byte* const alignedPtr = reinterpret_cast<std::byte*>(std::align(Alignment, sizeof(T), voidOffsetPtr, freeBytes));

            if (!alignedPtr)
            {
                throw std::out_of_range("Memorypool is full");
            }

            m_OffsetPtr = alignedPtr + sizeof(T);
            return Handle<T>(alignedPtr);
        }

        [[nodiscard]] std::byte* Allocate(size_t numBytes)
        {
            size_t freeBytes = m_Capacity - static_cast<size_t>(m_OffsetPtr - m_MemoryPool);
            void* voidOffsetPtr = static_cast<void*>(m_OffsetPtr);
            std::byte* const alignedPtr = reinterpret_cast<std::byte*>(std::align(Alignment, numBytes, voidOffsetPtr, freeBytes));

            if (!alignedPtr)
            {
                throw std::out_of_range("Memorypool is full");
            }

            m_OffsetPtr = alignedPtr + numBytes;
            return alignedPtr;
        }

        /**
         * @brief Allocates and copies the input data to the memory pool and returns a handle to the allocation.
         * @tparam T The type of the data to copy.
         * @param data The data to copy.
         * @return A handle to the allocation.
         */
        template<typename T>
        [[nodiscard]] Handle<T> Append(const T& data)
        {
            const Handle<T> handle = this->Allocate<T>();
            handle.Write(data);
            return handle;
        }

        /**
         * @brief Resets the offset to the beginning of the memory pool.
         */
        void Rewind() { m_OffsetPtr = m_MemoryPool; }

        /**
        * @brief Returns the number of free remaining bytes.
        * @return Number of free remaining bytes.
        */
        [[nodiscard]] size_t GetFreeBytes() const { return this->GetCapacity() - this->GetOffset(); }

        /**
         * @brief Returns the max capacity of the allocator in bytes.
         * @return Max number of bytes that can be allocated.
         */
        [[nodiscard]] size_t GetCapacity() const { return m_Capacity; }

        /**
         * @brief Returns the current offset in bytes into the allocator.
         * @return Offset in bytes.
         */
        [[nodiscard]] size_t GetOffset() const { return static_cast<size_t>(m_OffsetPtr - m_MemoryPool); }

        /**
         * @brief Returns whether or not the allocator references any memory.
         * @return TRUE if it references any memory.
         */
        bool IsValid() const { return (m_MemoryPool != nullptr && m_Capacity > 0); }

    private:
        std::byte* m_MemoryPool = nullptr;
        std::byte* m_OffsetPtr = nullptr;
        size_t m_Capacity = 0;

        void Move(LinearAllocator& other)
        {
            m_MemoryPool = other.m_MemoryPool;
            m_OffsetPtr = other.m_OffsetPtr;
            m_Capacity = other.m_Capacity;

            other.m_MemoryPool = nullptr;
            other.m_OffsetPtr = nullptr;
            other.m_Capacity = 0;
        }
    };
}