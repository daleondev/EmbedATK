#pragma once

#include <memory_resource>
#include <memory>
#include <algorithm>
#include <utility>
#include <array>
#include <span>
#include <ranges>
#include <deque>

//------------------------------------------------------
//                      Alloc Data
//------------------------------------------------------

struct AllocData
{
    size_t size     = 0;
    size_t align    = alignof(uintptr_t);
};

template<typename T>
static constexpr AllocData allocData() 
{ 
    return AllocData
    { 
        .size   = sizeof(T),
        .align  = std::max(alignof(T), alignof(uintptr_t))
    }; 
}

template <typename T>
struct MaxAllocData;

template <typename... T>
struct MaxAllocData<std::tuple<T...>> 
{
    static constexpr AllocData data = {
        .size = std::max({(allocData<T>().size)...}),
        .align = std::max({(allocData<T>().align)...})
    };
};

//------------------------------------------------------
//                      Buffer
//------------------------------------------------------

class IBuffer
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using ValueType     = std::byte;
    using Iterator      = std::span<ValueType>::iterator;
    using ConstIterator = std::span<const ValueType>::iterator;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    virtual ~IBuffer() = default;

    // ----------------------------------------
    // --- iterators
    // ----------------------------------------
    virtual constexpr Iterator begin() = 0;
    virtual constexpr ConstIterator begin() const = 0;
    virtual constexpr Iterator end() = 0;
    virtual constexpr ConstIterator end() const = 0;

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    virtual constexpr size_t size() const = 0;

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    virtual constexpr ValueType* data() = 0;
    virtual constexpr const ValueType* data() const = 0;
    virtual constexpr ValueType& operator[](const size_t index) = 0;
    virtual constexpr const ValueType& operator[](const size_t index) const = 0;
    virtual constexpr ValueType& at(size_t index) = 0;
    virtual constexpr const ValueType& at(size_t index) const = 0;

    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    virtual void clear(size_t index, size_t count) = 0;
    virtual void fill(size_t index, size_t count, const ValueType& value) = 0;
};

template<size_t N, size_t Align>
class StaticBuffer : public IBuffer
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Handle        = IBuffer;
    using ValueType     = Handle::ValueType;
    using Iterator      = Handle::Iterator;
    using ConstIterator = Handle::ConstIterator;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    StaticBuffer() = default;
    StaticBuffer(const StaticBuffer&) = default;
    StaticBuffer(StaticBuffer&&) = default;
    virtual ~StaticBuffer() = default;

    // ----------------------------------------
    // --- iterators
    // ----------------------------------------
    virtual constexpr Iterator begin() { return std::span<ValueType>{m_data.data(), m_data.size()}.begin(); }
    virtual constexpr ConstIterator begin() const { return std::span<const ValueType>{m_data.data(), m_data.size()}.begin(); }
    virtual constexpr Iterator end() { return std::span<ValueType>{m_data.data(), m_data.size()}.end(); }
    virtual constexpr ConstIterator end() const { return std::span<const ValueType>{m_data.data(), m_data.size()}.end(); }

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    constexpr size_t size() const override { return m_data.size(); }

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    constexpr ValueType* data() override { return m_data.data(); }
    constexpr const ValueType* data() const override { return m_data.data(); }
    constexpr ValueType& operator[](const size_t index) override { return m_data[index]; }
    constexpr const ValueType& operator[](const size_t index) const override { return m_data[index]; }
    constexpr ValueType& at(const size_t index) override { return m_data.at(index); }
    constexpr const ValueType& at(const size_t index) const override { return m_data.at(index); }
    
    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    StaticBuffer& operator=(const StaticBuffer&) = default;
    StaticBuffer& operator=(StaticBuffer&&) = default;

    void clear(size_t index, size_t count) override
    {
        std::ranges::fill(m_data | std::views::drop(index) | std::views::take(count), ValueType{0x00});
    }

    void fill(size_t index, size_t count, const ValueType& value) override
    {
        std::ranges::fill(m_data | std::views::drop(index) | std::views::take(count), value);
    }

protected:
    alignas(Align) std::array<ValueType, N> m_data = {};
};

template<size_t N, AllocData Block>
class StaticBlockBuffer : public StaticBuffer<Block.size*N, Block.align>
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Buffer        = StaticBuffer<Block.size*N, Block.align>;
    using ValueType     = Buffer::ValueType;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    StaticBlockBuffer() = default;
    StaticBlockBuffer(const StaticBlockBuffer&) = default;
    StaticBlockBuffer(StaticBlockBuffer&&) = default;
    virtual ~StaticBlockBuffer() = default;

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    constexpr ValueType& atBlock(const size_t index) { return Buffer::at(Block.size*index); }
    constexpr const ValueType& atBlock(const size_t index) const { return Buffer::at(Block.size*index); }
    
    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    StaticBlockBuffer& operator=(const StaticBlockBuffer&) = default;
    StaticBlockBuffer& operator=(StaticBlockBuffer&&) = default;

    void clearBlock(size_t index, size_t count)
    {
        Buffer::clear(Block.size*index, Block.size*count);
    }

    void fillBlock(size_t index, size_t count, const ValueType& value)
    {
        Buffer::fill(Block.size*index, Block.size*count, value);
    }
};

//------------------------------------------------------
//                      ObjectStore
//------------------------------------------------------

template<typename T>
class IObjectStore
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using ValueType     = T;
    using Iterator      = std::span<ValueType>::iterator;
    using ConstIterator = std::span<const ValueType>::iterator;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    virtual ~IObjectStore() = default;

    // ----------------------------------------
    // --- iterators
    // ----------------------------------------
    virtual constexpr Iterator begin() = 0;
    virtual constexpr ConstIterator begin() const = 0;
    virtual constexpr Iterator end() = 0;
    virtual constexpr ConstIterator end() const = 0;

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    virtual constexpr size_t size() const = 0;

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    virtual constexpr ValueType* data() = 0;
    virtual constexpr const ValueType* data() const = 0;
    virtual constexpr ValueType& operator[](const size_t index) = 0;
    virtual constexpr const ValueType& operator[](const size_t index) const = 0;
    virtual constexpr ValueType& at(size_t index) = 0;
    virtual constexpr const ValueType& at(size_t index) const = 0;

    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    virtual void destroy(size_t index, size_t count) = 0;
    virtual void construct(size_t index, size_t count) = 0;
    virtual void fill(size_t index, size_t count, const ValueType& value) = 0;
    virtual void clone(const IObjectStore& other, size_t index, size_t count) = 0;
    virtual void transfer(IObjectStore&& other, size_t index, size_t count) = 0;
    virtual void uninitializedFill(size_t index, size_t count, const ValueType& value) = 0;
    virtual void uninitializedClone(const IObjectStore& other, size_t index, size_t count) = 0;
    virtual void uninitializedTransfer(IObjectStore&& other, size_t index, size_t count) = 0;
};

template<typename T, size_t N, bool ClearOnDestroy = true>
class StaticObjectStore : public IObjectStore<T>
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Handle        = IObjectStore<T>;
    using ValueType     = Handle::ValueType;
    using Iterator      = Handle::Iterator;
    using ConstIterator = Handle::ConstIterator;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    StaticObjectStore() : m_buff{}, m_data{reinterpret_cast<ValueType*>(m_buff.data()), N} {}
    StaticObjectStore(const StaticObjectStore&) = delete;
    StaticObjectStore(StaticObjectStore&&) = delete;
    virtual ~StaticObjectStore() = default;

    // ----------------------------------------
    // --- iterators
    // ----------------------------------------
    constexpr Iterator begin() override { return m_data.begin(); }
    constexpr ConstIterator begin() const override { return std::span<const ValueType>{m_data}.begin(); }
    constexpr Iterator end() override { return m_data.end(); }
    constexpr ConstIterator end() const override { return std::span<const ValueType>{m_data}.end(); }

    // ----------------------------------------
    // --- information
    // ----------------------------------------
     constexpr size_t size() const override { return m_data.size(); }

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    constexpr ValueType* data() override { return m_data.data(); }
    constexpr const ValueType* data() const override { return m_data.data(); }
    constexpr ValueType& operator[](const size_t index) override { return m_data[index]; }
    constexpr const ValueType& operator[](const size_t index) const override { return m_data[index]; }

    constexpr ValueType& at(size_t index) override 
    { 
        if (index >= N) throw std::out_of_range("index exceeds vector size");
        return m_data[index]; 
    }

    constexpr const ValueType& at(size_t index) const override 
    { 
        if (index >= N) throw std::out_of_range("index exceeds vector size");
        return m_data[index]; 
    }

    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    void destroy(size_t index, size_t count) override
    {
        std::ranges::destroy(m_data | std::views::drop(index) | std::views::take(count));
        if constexpr (ClearOnDestroy) m_buff.clearBlock(index, count);
    }

    void construct(size_t index, size_t count) override
    {
        std::uninitialized_value_construct_n(data() + index, count);
    }

    void fill(size_t index, size_t count, const ValueType& value) override
    {
        std::ranges::fill(m_data | std::views::drop(index) | std::views::take(count), value);
    }

    void clone(const Handle& other, size_t index, size_t count) override
    {
        std::ranges::copy(other | std::views::drop(index) | std::views::take(count), begin()+index);
    }

    void transfer(Handle&& other, size_t index, size_t count) override
    {
        std::ranges::move(other | std::views::drop(index) | std::views::take(count), begin()+index);
    }

    void uninitializedFill(size_t index, size_t count, const ValueType& value) override
    {
        std::uninitialized_fill_n(data() + index, count, value);
    }

    void uninitializedClone(const IObjectStore<ValueType>& other, size_t index, size_t count) override
    {
        std::uninitialized_copy_n(other.data() + index, count, begin() + index);
    }

    void uninitializedTransfer(IObjectStore<ValueType>&& other, size_t index, size_t count) override
    {
        std::uninitialized_move_n(other.data() + index, count, begin() + index);
    }

private:
    // ----------------------------------------
    // --- data
    // ----------------------------------------
    StaticBlockBuffer<N, allocData<ValueType>()> m_buff;
    std::span<T> m_data;
};

//------------------------------------------------------
//                    PMR Unique Ptr
//------------------------------------------------------

template <typename T>
struct StaticDeleter {
    std::pmr::memory_resource* resource;

    StaticDeleter(std::pmr::memory_resource* res = nullptr) : resource(res) {}

    template <typename U>
    requires std::convertible_to<U*, T*>
    StaticDeleter(const StaticDeleter<U>& other) : resource(other.resource) {}

    void operator()(T* ptr) const 
    {
        if (ptr) {
            ptr->~T();
            resource->deallocate(ptr, sizeof(T), alignof(T));
        }
    }
};

template <typename T>
using StaticUniquePtr = std::unique_ptr<T, StaticDeleter<T>>;

template <typename T, typename... Args>
constexpr StaticUniquePtr<T> make_static_unique(std::pmr::memory_resource* resource, Args&&... args) {
    void* mem = resource->allocate(sizeof(T), alignof(T));
    T* ptr = new (mem) T(std::forward<Args>(args)...);
    return StaticUniquePtr<T>(ptr, StaticDeleter<T>{resource});
}

//------------------------------------------------------
//                    Pools
//------------------------------------------------------

class IPool : public std::pmr::memory_resource
{
public:
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

template<size_t N, AllocData Alloc>
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

    StaticBuffer<MapSize == InitialMapSize ? MapSize : 2*MapSize, alignof(uintptr_t)> m_mapBuff;
    StaticBuffer<DataSize, alignof(T)> m_blockBuff;
    FreeBlock* m_freeBlocks;
};
