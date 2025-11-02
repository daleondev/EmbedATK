#pragma once

#include "Buffer.h"

//------------------------------------------------------
//                    Pools
//------------------------------------------------------

class IPool : public std::pmr::memory_resource
{
public:
    virtual ~IPool() = default;
    virtual void* data() = 0;
    virtual const void* data() const = 0;
};

template<size_t N, AllocData Block>
class StaticBlockPool : public IPool
{
    struct FreeBlock {
        FreeBlock* next = nullptr;
    };

    static_assert(Block.size >= sizeof(FreeBlock),
	  "invalid block size, size needs to be at least the size of a pointer");

public:
    StaticBlockPool() 
        : m_buff{}, m_freeBlocks{nullptr}
    {
        for (size_t i = N; i > 0; --i) {
            auto* freeBlock = reinterpret_cast<FreeBlock*>(&m_buff.atBlock(i-1));
            freeBlock->next = m_freeBlocks;
            m_freeBlocks = freeBlock;
        }
    }

    StaticBlockPool(const StaticBlockPool&) = delete;
    StaticBlockPool& operator=(const StaticBlockPool&) = delete;
    StaticBlockPool(StaticBlockPool&&) = default;
    StaticBlockPool& operator=(StaticBlockPool&&) = default;

    ~StaticBlockPool() = default;

    void* data() override { return m_buff.data(); }
    const void* data() const override{ return m_buff.data(); }

    template<typename T, typename ...Args>
    T* construct(Args&&... args) 
    {
        static_assert(allocData<T>() == Block);
        auto* ptr = static_cast<T*>(std::pmr::memory_resource::allocate(Block.size, Block.align));
        std::construct_at<T>(ptr, std::forward<Args>(args)...);
        return ptr;
    }

    template<typename T>
    void destroy(T* ptr)
    {
        static_assert(allocData<T>() == Block);
        std::destroy_at(ptr);
        std::pmr::memory_resource::deallocate(ptr, Block.size, Block.align);
    }

private:
    void* do_allocate(size_t bytes, size_t alignment) override 
    {
        if (m_freeBlocks == nullptr || bytes > Block.size || alignment > Block.align) [[unlikely]] {
            throw std::bad_alloc{};
        }

        FreeBlock* allocatedBlock = m_freeBlocks;
        m_freeBlocks = allocatedBlock->next;

        return allocatedBlock;
    }

    void do_deallocate(void* p, size_t bytes, size_t) override 
    {
        const auto addrToFree = reinterpret_cast<uintptr_t>(p);
        const auto startAddr = reinterpret_cast<uintptr_t>(m_buff.data());
        const auto endAddr = startAddr + (N*Block.size);

        if (p == nullptr || addrToFree < startAddr || (addrToFree+bytes) > endAddr) [[unlikely]] {
            throw std::invalid_argument("Deallocation error");
        }

        const size_t index = addrToFree - startAddr;
        m_buff.clear(index, Block.size);

        auto* freedBlock = static_cast<FreeBlock*>(p);
        freedBlock->next = m_freeBlocks;
        m_freeBlocks = freedBlock;
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override 
    {
        return this == &other;
    }

    StaticBlockBuffer<N, Block> m_buff;
    FreeBlock* m_freeBlocks;
};

template<typename T, size_t N>
class StaticEntiredPool : public IPool
{
public:
    StaticEntiredPool() = default;

    StaticEntiredPool(const StaticEntiredPool&) = delete;
    StaticEntiredPool& operator=(const StaticEntiredPool&) = delete;
    StaticEntiredPool(StaticEntiredPool&&) = default;
    StaticEntiredPool& operator=(StaticEntiredPool&&) = default;

    ~StaticEntiredPool() = default;

    void* data() override { return m_buff.data(); }
    const void* data() const override{ return m_buff.data(); }

private:
    void* do_allocate(size_t bytes, size_t alignment) override 
    {
        if (bytes > N*Alloc.size || alignment > Alloc.align) [[unlikely]] {
            throw std::bad_alloc{};
        }

        void* ptr = m_buff.data();
        size_t space = N*Alloc.size;
        if (std::align(alignment, bytes, ptr, space)) {
            return ptr;
        }

        throw std::bad_alloc{};
    }

    void do_deallocate(void* p, size_t bytes, size_t) override 
    {
        const auto addrToFree = reinterpret_cast<uintptr_t>(p);
        const auto startAddr = reinterpret_cast<uintptr_t>(m_buff.data());

        if (p == nullptr || addrToFree != startAddr || bytes > N*Alloc.size) [[unlikely]] {
            throw std::invalid_argument("Deallocation error");
        }

        const size_t index = addrToFree - startAddr;
        m_buff.clear(index, bytes);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override 
    {
        return this == &other;
    }

private:
    static constexpr AllocData Alloc = allocData<T>();
    StaticBuffer<Alloc.size*N, Alloc.align> m_buff;
};

template<typename T, size_t N>
class StaticMonotonicPool : public IPool, public std::pmr::monotonic_buffer_resource
{
    using Resource = std::pmr::monotonic_buffer_resource;

public:
    StaticMonotonicPool()
        : Resource(m_buff.data(), m_buff.size())
    {}

    StaticMonotonicPool(const StaticMonotonicPool&) = delete;
    StaticMonotonicPool& operator=(const StaticMonotonicPool&) = delete;
    StaticMonotonicPool(StaticMonotonicPool&&) = default;
    StaticMonotonicPool& operator=(StaticMonotonicPool&&) = default;

    ~StaticMonotonicPool() = default;

    void* data() override { return m_buff.data(); }
    const void* data() const override{ return m_buff.data(); }

    void* allocate(size_t bytes, size_t alignment) 
    {
        return Resource::allocate(bytes, alignment);
    }

    void deallocate(void* p, size_t bytes, size_t alignment) 
    {
        Resource::deallocate(p, bytes, alignment);
    }

    bool is_equal(const std::pmr::memory_resource& other) const noexcept 
    {
        return Resource::is_equal(other);
    }

private:
    void* do_allocate(size_t bytes, size_t alignment) override 
    {
        void* p = Resource::do_allocate(bytes, alignment);

        const auto allocatedAddr = reinterpret_cast<uintptr_t>(p);
        const auto startAddr = reinterpret_cast<uintptr_t>(m_buff.data());
        const auto endAddr = startAddr + (N*Alloc.size);
        if (p == nullptr || allocatedAddr < startAddr || (allocatedAddr + bytes) > endAddr) [[unlikely]] {
            throw std::bad_alloc{};
        }

        return p;
    }

    void do_deallocate(void* p, size_t bytes, size_t alignment) override 
    {
        Resource::do_deallocate(p, bytes, alignment);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override 
    {
        return Resource::do_is_equal(other);
    }

    static constexpr AllocData Alloc = allocData<T>();
    StaticBuffer<N*Alloc.size, Alloc.align> m_buff = {};
};

template<typename T, size_t N>
class StaticDequePool : public IPool
{
    struct FreeBlock {
        FreeBlock* next = nullptr;
    };

#if defined(__GLIBCXX__)
    struct DequeData : public std::pmr::deque<T>
    {
        inline static constexpr size_t NumElementsInBlock = std::__deque_buf_size(sizeof(T));
        inline static constexpr size_t NumNodes = (N == 0) ? 0 : ((N - 1) / NumElementsInBlock + 1);
        inline static constexpr size_t InitialMapSize = static_cast<size_t>(std::pmr::deque<T>::_Deque_base::_S_initial_map_size);
        inline static constexpr size_t MapSize = std::max(InitialMapSize, NumNodes + 2) * sizeof(void*);
        inline static constexpr size_t DataSize = NumNodes*sizeof(T)*std::__deque_buf_size(sizeof(T));
        inline static constexpr size_t AllocSize = MapSize + DataSize;
    };
#elif defined(_LIBCPP_VERSION)
    namespace detail
    {
        inline constexpr size_t __libcxx_deque_buf_size(size_t s)
        {
            if (s == 0) return 16;
            return std::max(static_cast<size_t>(16), 4096 / s);
        }
        inline constexpr size_t __libcxx_initial_map_size = 2;
    }

    struct DequeData : public std::pmr::deque<T>
    {
        inline static constexpr size_t NumElementsInBlock = detail::__libcxx_deque_buf_size(sizeof(T));
        inline static constexpr size_t NumNodes = (N == 0) ? 0 : ((N - 1) / BlockSizeInElements + 1);
        inline static constexpr size_t InitialMapSize = detail::__libcxx_initial_map_size;
        inline static constexpr size_t MapSize = std::max(InitialMapSize, NumNodes + 2) * sizeof(void*);
        inline static constexpr size_t DataSize = NumNodes * BlockSizeInElements * sizeof(T);
        inline static constexpr size_t AllocSize = MapSize + DataSize;
    };
#endif

    static constexpr size_t NumBlocks       = DequeData::NumNodes;
    static constexpr size_t BlockSize       = DequeData::NumElementsInBlock * sizeof(T);
    static constexpr size_t InitialMapSize  = DequeData::InitialMapSize;
    static constexpr size_t MapSize         = DequeData::MapSize;
    static constexpr size_t DataSize        = DequeData::DataSize;

    static_assert(BlockSize >= sizeof(FreeBlock),
	  "invalid block size, size needs to be at least the size of a pointer");

public:
    StaticDequePool() 
        : m_mapBuff{}, m_blockBuff{}, m_freeBlocks{nullptr}
    {
        if constexpr (NumBlocks > 0) {
            void* blockStart = m_blockBuff.data();
            
            for (size_t i = NumBlocks; i > 0; --i) {
                void* blockAddr = static_cast<std::byte*>(blockStart) + ((i - 1) * BlockSize);
                auto* freeBlock = static_cast<FreeBlock*>(blockAddr);
                freeBlock->next = m_freeBlocks;
                m_freeBlocks = freeBlock;
            }
        }
    }

    StaticDequePool(const StaticDequePool&) = delete;
    StaticDequePool& operator=(const StaticDequePool&) = delete;
    StaticDequePool(StaticDequePool&&) = default;
    StaticDequePool& operator=(StaticDequePool&&) = default;

    ~StaticDequePool() = default;

    void* data() override { return &m_mapBuff[0]; }
    const void* data() const override { return &m_mapBuff[0]; }

private:
    void* do_allocate(size_t bytes, size_t alignment) override 
    {
        if (bytes == BlockSize && alignment <= alignof(T) && m_freeBlocks != nullptr) {
            FreeBlock* allocatedBlock = m_freeBlocks;
            m_freeBlocks = allocatedBlock->next;
            return allocatedBlock;
        }

        if (bytes != BlockSize && bytes <= m_mapBuff.size() && alignment <= alignof(uintptr_t)) {
            return &m_mapBuff[0];
        }

        [[unlikely]] throw std::bad_alloc{};
    }

    void do_deallocate(void* p, size_t bytes, size_t alignment) override 
    {
        if (p == nullptr) {
            return;
        }

        const auto addrToFree = reinterpret_cast<uintptr_t>(p);
        const auto mapStartAddr = reinterpret_cast<uintptr_t>(&m_mapBuff[0]);
        if (addrToFree == mapStartAddr) {
            return;
        }

        if constexpr (NumBlocks > 0) {
            const auto blockStartAddr = reinterpret_cast<uintptr_t>(&m_blockBuff[0]);
            const auto blockEndAddr = blockStartAddr + DataSize;

            if (addrToFree >= blockStartAddr && addrToFree < blockEndAddr) {
                auto* freedBlock = static_cast<FreeBlock*>(p);
                freedBlock->next = m_freeBlocks;
                m_freeBlocks = freedBlock;
                return;
            }
        }

        [[unlikely]] throw std::invalid_argument("Deallocation error");
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override 
    {
        return this == &other;
    }

    StaticBuffer<MapSize == (InitialMapSize*sizeof(uintptr_t)) ? MapSize : 2*MapSize, alignof(uintptr_t)> m_mapBuff;
    StaticBuffer<DataSize, alignof(T)> m_blockBuff;
    FreeBlock* m_freeBlocks;
};