#pragma once

#include "Container.h"

#include "EmbedATK/Memory/Pool.h"
#include "EmbedATK/Memory/Store.h"

//------------------------------------------------------
//                      Queue
//------------------------------------------------------

template<typename T>
class IQueue : public ISequentialContainer<T>
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Container     = ISequentialContainer<T>;
    using ValueType     = Container::ValueType;
    using Iterator      = Container::Iterator;
    using ConstIterator = Container::ConstIterator;
    using OptionalRef   = std::optional<std::reference_wrapper<ValueType>>;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    virtual ~IQueue() = default;

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    virtual constexpr bool full() const noexcept = 0;
    virtual constexpr size_t capacity() const noexcept = 0;

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    virtual std::optional<std::reference_wrapper<const ValueType>> peek() const = 0;

    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    virtual void clear() noexcept = 0;
    virtual void resize(size_t size) = 0;
    virtual void resize(size_t size, const ValueType& value) = 0;
    virtual bool push(const ValueType& value) = 0;
    virtual bool push(ValueType&& value) = 0;
    virtual std::optional<ValueType> pop() = 0;
    virtual Iterator insert(ConstIterator pos, Iterator first, Iterator last) = 0;
    virtual Iterator insert(ConstIterator pos, std::move_iterator<Iterator> first, std::move_iterator<Iterator> last) = 0;
    virtual Iterator erase(size_t index) = 0;
    virtual Iterator erase(size_t index, size_t count) = 0;
    virtual Iterator erase(ConstIterator pos) = 0;
    virtual Iterator erase(ConstIterator first, ConstIterator last) = 0;

    void swap(IQueue<ValueType>& other) 
    { 
        if (this->size() > other.capacity() || other.size() > capacity()) 
            throw std::length_error("Invalid queue sizes for swap");

        const auto minSize = std::min(this->size(), other.size());
        std::ranges::swap_ranges(other | std::views::take(minSize), *this | std::views::take(minSize));

        if (other.size() > this->size()) {
            resize(other.size());
            std::ranges::move(other | std::views::drop(minSize), this->begin() + minSize);
            other.resize(minSize);
        }
        else if (this->size() > other.size()) {
            other.resize(this->size());
            std::ranges::move(*this | std::views::drop(minSize), other.begin() + minSize);
            resize(minSize);
        }
    }
};

template<typename T, size_t N>
requires std::default_initializable<T>
class StaticStdQueue : public IQueue<T>
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Handle                = IQueue<T>;
    using ValueType             = Handle::ValueType;
    using Iterator              = Handle::Iterator;
    using ConstIterator         = Handle::ConstIterator;
    using OptionalRef           = Handle::OptionalRef;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    StaticStdQueue() 
        : m_pool{}, m_queue{&m_pool}
    { 
    }
    StaticStdQueue(size_t size)
        : m_pool{}, m_queue{&m_pool}
    {
        if (size > N) throw std::length_error("queue exceeds static capacity");
        m_queue.resize(size);
    }
    StaticStdQueue(size_t size, const ValueType& val) 
        : m_pool{}, m_queue{&m_pool}
    {
        if (size > N) throw std::length_error("queue exceeds static capacity");
        m_queue.resize(size, val);
    }

    StaticStdQueue(const StaticStdQueue& other) 
        : m_pool{}, m_queue{other.m_queue, &m_pool}
    {}

    StaticStdQueue(StaticStdQueue&& other) noexcept 
        : m_pool{}, m_queue{std::move(other.m_queue), &m_pool}
    {}

    ~StaticStdQueue() = default;

    // ----------------------------------------
    // --- iterators
    // ----------------------------------------
    Iterator begin() override { return Iterator(m_queue.begin()); }
    ConstIterator begin() const override { return ConstIterator(m_queue.cbegin()); }
    Iterator end() override { return Iterator(m_queue.end()); }
    ConstIterator end() const override { return ConstIterator(m_queue.cend()); }

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    bool empty() const noexcept override { return m_queue.empty(); }
    bool full() const noexcept override { return m_queue.size() == N; }
    size_t size() const noexcept override { return m_queue.size(); }
    size_t capacity() const noexcept override { return N; }
    
    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    ValueType& operator[](const size_t i) noexcept override { return m_queue[i]; }
    const ValueType& operator[](const size_t i) const noexcept override { return m_queue[i]; }
    ValueType& at(size_t i) override { return m_queue.at(i); }
    const ValueType& at(size_t i) const override { return m_queue.at(i); }

    std::optional<std::reference_wrapper<const ValueType>> peek() const override
    { 
        if (empty()) return std::nullopt;
        return m_queue.front();
    }
    
    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    StaticStdQueue& operator=(const StaticStdQueue& other)
    {
        if (this != &other) {
            m_queue.clear();
            m_queue.resize(other.m_queue.size());
            std::ranges::copy(other.m_queue, m_queue.begin());
        }
        return *this;
    }

    StaticStdQueue& operator=(StaticStdQueue&& other) noexcept
    {
        if (this != &other) {
            m_queue.clear();
            m_queue.resize(other.m_queue.size());
            std::ranges::move(other.m_queue, m_queue.begin());
            other.m_queue.clear();
        }
        return *this;
    }

    virtual void clear() noexcept override { m_queue.clear(); }

    virtual void resize(size_t size) override 
    { 
        if (size > N) throw std::length_error("queue exceeds static capacity");
        m_queue.resize(size); 
    }

    virtual void resize(size_t size, const ValueType& value) override 
    { 
        if (size > N) throw std::length_error("queue exceeds static capacity");
        m_queue.resize(size, value); 
    }

    virtual bool push(const ValueType& value) override 
    { 
        if constexpr (std::is_copy_constructible_v<ValueType>)
        {
            if (full()) return false;
            m_queue.push_back(value); 
            return true;
        }
        else
        {
            return false;
        }
    }

    virtual bool push(ValueType&& value) override 
    { 
        if (full()) return false;
        m_queue.push_back(std::move(value)); 
        return true; 
    }

    virtual std::optional<ValueType> pop() override 
    {  
        if (empty()) return std::nullopt;
        ValueType value = std::move(m_queue.front());
        m_queue.pop_front();
        return std::move(value);
    }

    virtual Iterator insert(ConstIterator pos, Iterator first, Iterator last) override 
    { 
        const auto copyCount = std::distance(first, last);
        if (m_queue.size() + copyCount > N) throw std::length_error("Insert would exceed static queue capacity");

        const auto index = std::distance(ConstIterator(begin()), pos);
        return Iterator(m_queue.insert(begin()+index, first, last));
    }

    virtual Iterator insert(ConstIterator pos, std::move_iterator<Iterator> first, std::move_iterator<Iterator> last) override 
    { 
        const auto copyCount = std::distance(first, last);
        if (m_queue.size() + copyCount > N) throw std::length_error("Insert would exceed static queue capacity");

        const auto index = std::distance(ConstIterator(begin()), pos);
        return Iterator(m_queue.insert(begin()+index, first, last));
    }

    Iterator erase(size_t index) override 
    {
        return Iterator(m_queue.erase(m_queue.begin()+index));
    }

    Iterator erase(size_t index, size_t count) override 
    {
        return Iterator(m_queue.erase(m_queue.begin()+index, m_queue.begin()+index+count));
    }

    Iterator erase(ConstIterator pos) override 
    {
        const auto index = std::distance(ConstIterator(begin()), pos);
        return erase(index);
    }

    Iterator erase(ConstIterator first, ConstIterator last) override 
    {
        const auto index = std::distance(ConstIterator(begin()), first);
        const auto count = std::distance(first, last);
        return erase(index, count);
    }

    template<class... Args>
    OptionalRef emplace(Args&&... args)
    {
        if (full()) return std::nullopt;
        return std::ref(m_queue.emplace_back(std::forward<Args...>(args...)));
    }

private:
    // ----------------------------------------
    // --- data
    // ----------------------------------------
    StaticDequePool<ValueType, N> m_pool;
    std::pmr::deque<ValueType> m_queue;
};

static_assert(std::ranges::random_access_range<StaticStdQueue<int, 5>>);
static_assert(std::ranges::random_access_range<const StaticStdQueue<int, 5>>);

template<typename T>
requires std::default_initializable<T>
class StaticQueueBase : public IQueue<T>
{   
    static_assert(std::is_same<typename std::remove_cv<T>::type, T>::value,
	  "StaticQueueBase must have a non-const, non-volatile value_type");

    template<typename T_friend>
    requires std::default_initializable<T_friend>
    friend class StaticQueueBase;

    template<bool IsConst>
    struct QueueIterator 
    {
        // ----------------------------------------
        // --- types
        // ----------------------------------------
        using iterator_category = std::random_access_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = std::remove_const_t<T>;
        using pointer           = std::conditional_t<IsConst, const value_type*, value_type*>;
        using reference         = std::conditional_t<IsConst, const value_type&, value_type&>;
        using owner_type        = std::conditional_t<IsConst, const StaticQueueBase, StaticQueueBase>;

        // ----------------------------------------
        // --- constructors/destructors
        // ----------------------------------------
        constexpr QueueIterator() = default;
        constexpr QueueIterator(owner_type* owner, size_t index) : owner{ owner }, index{ index } {}
        template<bool WasConst>
        requires(IsConst && !WasConst)
        constexpr QueueIterator(const QueueIterator<WasConst>& other) : owner{ other.owner }, index{ other.index } {}
        ~QueueIterator() = default;

        // --- C++23: Let the compiler handle most of this! ---
        // The spaceship operator (<=>) generates ==, !=, <, >, <=, >= for free.
        auto operator<=>(const QueueIterator&) const = default;

        // ----------------------------------------
        // --- data access
        // ----------------------------------------
        reference operator*() const { return (*owner)[index]; }
        reference operator[](difference_type n) const { return (*owner)[index + n]; }

        // ----------------------------------------
        // --- manipulation
        // ----------------------------------------
        QueueIterator& operator++() { ++index; return *this; }
        QueueIterator operator++(int) { QueueIterator tmp = *this; ++index; return tmp; }
        QueueIterator& operator--() { --index; return *this; }
        QueueIterator operator--(int) { QueueIterator tmp = *this; --index; return tmp; }
        QueueIterator& operator+=(difference_type n) { index += n; return *this; }
        QueueIterator& operator-=(difference_type n) { index -= n; return *this; }

        friend QueueIterator operator+(QueueIterator it, difference_type n) { return (it += n); }
        friend QueueIterator operator+(difference_type n, QueueIterator it) { return (it += n); }
        friend QueueIterator operator-(QueueIterator it, difference_type n) { return (it -= n); }
        friend difference_type operator-(const QueueIterator& lhs, const QueueIterator& rhs) {
            return static_cast<difference_type>(lhs.index) - static_cast<difference_type>(rhs.index);
        }
        
        // ----------------------------------------
        // --- data
        // ----------------------------------------
        owner_type* owner;
        size_t index;
    };

public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Handle                = IQueue<T>;
    using ValueType             = Handle::ValueType;
    using Iterator              = Handle::Iterator;
    using ConstIterator         = Handle::ConstIterator;
    using InternalIterator      = QueueIterator<false>;
    using InternalConstIterator = QueueIterator<true>;
    using OptionalRef           = Handle::OptionalRef;
    using StoreType             = IObjectStore<ValueType>;
    
    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    StaticQueueBase(size_t size, size_t head, size_t tail)
        : m_size(size), m_head(head), m_tail(tail)
    {}
    virtual ~StaticQueueBase() = default;

    // ----------------------------------------
    // --- iterators
    // ----------------------------------------
    constexpr Iterator begin() noexcept override { return Iterator(InternalIterator{this, 0}); }
    constexpr ConstIterator begin() const noexcept override { return ConstIterator(InternalConstIterator{this, 0}); }
    constexpr Iterator end() noexcept override { return Iterator(InternalIterator{this, m_size}); }
    constexpr ConstIterator end() const noexcept override { return ConstIterator(InternalConstIterator{this, m_size}); }

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    constexpr bool empty() const noexcept override { return m_size == 0; }
    constexpr bool full() const noexcept override { return m_size >= getStore().size(); }
    constexpr size_t size() const noexcept override { return m_size; }
    constexpr size_t capacity() const noexcept override { return getStore().size(); }

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    ValueType& operator[](size_t index) override { return data()[(m_head + index) % capacity()]; }
    const ValueType& operator[](size_t index) const override { return data()[(m_head + index) % capacity()]; }

    ValueType& at(size_t index) override
    { 
        if (index >= m_size) throw std::out_of_range("index exceeds queue size");
        return (*this)[index];
    }

    const ValueType& at(size_t index) const override
    { 
        if (index >= m_size) throw std::out_of_range("index exceeds queue size");
        return (*this)[index];
    }

    std::optional<std::reference_wrapper<const ValueType>> peek() const override
    {
        if (empty()) return std::nullopt;
        return data()[m_head];
    }

    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    void clear() noexcept override
    {
        if (m_head > m_tail) {
            getStore().destroy(m_tail, capacity()-m_tail);
            getStore().destroy(0, m_head);
        } else {
            getStore().destroy(m_head, m_tail-m_head);
        }
        m_size = 0;
        m_head = 0;
        m_tail = 0;
    }

    void resize(size_t size) override
    {
        if (size > capacity()) throw std::length_error("Initial size exceeds static capacity"); 
        if (size == m_size) return;

        auto tail = (m_head + size) % capacity();
        if (size > m_size) {
            if (m_tail > tail) {
                getStore().construct(m_tail, capacity()-m_tail);
                getStore().construct(0, tail);
            } else {
                getStore().construct(m_tail, tail-m_tail);
            }
        }
        else {
            if (tail > m_tail) {
                getStore().destroy(tail, capacity()-tail);
                getStore().destroy(0, m_tail);
            } else {
                getStore().destroy(tail, m_tail-tail);
            }
        }
        m_size = size;
        m_tail = tail;
    }

    void resize(size_t size, const ValueType& value) override
    {
        if (size > capacity()) throw std::length_error("Initial size exceeds static capacity"); 
        if (size == m_size) return;

        auto tail = (m_head + size) % capacity();
        if (size > m_size) {
            if (m_tail > tail) {
                getStore().uninitializedFill(m_tail, capacity()-m_tail, value);
                getStore().uninitializedFill(0, tail, value);
            } else {
                getStore().uninitializedFill(m_tail, tail-m_tail, value);
            }
        }
        else {
            if (tail > m_tail) {
                getStore().destroy(tail, capacity()-tail);
                getStore().destroy(0, m_tail);
            } else {
                getStore().destroy(tail, m_tail-tail);
            }
        }
        m_size = size;
        m_tail = tail;
    }

    bool push(const ValueType& value) override
    {
        if constexpr (std::is_copy_constructible_v<ValueType>) {
            if (full()) return false;
            std::construct_at(&data()[m_tail], value);
            m_tail = (m_tail + 1) % capacity();
            m_size++;
            return true;
        }
        else {
            return false;
        }
    }

    bool push(ValueType&& value) override
    {
        if constexpr (std::is_move_constructible_v<ValueType>) {
            if (full()) return false;
            std::construct_at(&data()[m_tail], std::move(value));
            m_tail = (m_tail + 1) % capacity();
            m_size++;
            return true;
        }
        else {
            return false;
        }
    }

    std::optional<ValueType> pop() override
    {
        if (empty()) return std::nullopt;
        ValueType value = std::move(data()[m_head]);
        getStore().destroy(m_head, 1);
        m_head = (m_head + 1) % capacity();
        m_size--;
        return std::move(value);
    }

    Iterator insert(ConstIterator pos, Iterator first, Iterator last) override
    {
        if constexpr (std::is_copy_constructible_v<ValueType>) {
            const auto copyStart = std::distance(ConstIterator(begin()), pos);
            const auto copyCount = std::distance(first, last);
            const auto copyEnd = copyStart + copyCount;

            auto mutPos = Iterator(begin()+copyStart);
            if (copyCount == 0) return mutPos;
            if (m_size + copyCount > capacity()) throw std::length_error("Insert would exceed static queue capacity");

            if (pos != ConstIterator(end())) {
                // shift existing elements 
                std::uninitialized_move_n(end()-copyCount, copyCount, end());
                std::ranges::move(mutPos, end()-copyCount, begin()+copyEnd);

                // copy new elements
                if (copyEnd > m_size) {
                    std::copy_n(first, m_size-copyStart, mutPos);
                    std::uninitialized_copy(first + (m_size-copyStart), last, end());
                }
                else {
                    std::copy(first, last, mutPos);
                }
            }
            else {
                // copy new elements
                std::uninitialized_copy(first, last, mutPos);
            }
            
            m_size += copyCount;
            m_tail = (m_head + m_size) % capacity();
            
            return mutPos;
        }
        else {
            return end();
        }
    }

    Iterator insert(ConstIterator pos, std::move_iterator<Iterator> first, std::move_iterator<Iterator> last) override
    {
        if constexpr (std::is_move_constructible_v<ValueType>) {
            const auto copyStart = std::distance(ConstIterator(begin()), pos);
            const auto copyCount = std::distance(first, last);
            const auto copyEnd = copyStart + copyCount;

            auto mutPos = Iterator(begin()+copyStart);
            if (copyCount == 0) return mutPos;
            if (m_size + copyCount > capacity()) throw std::length_error("Insert would exceed static queue capacity");

            if (pos != ConstIterator(end())) {
                // shift existing elements 
                std::uninitialized_move_n(end()-copyCount, copyCount, end());
                std::ranges::move(mutPos, end()-copyCount, begin()+copyEnd);

                // move new elements
                if (copyEnd > m_size) {
                    std::ranges::move(first, first + (m_size-copyStart), mutPos);
                    std::uninitialized_move(first + (m_size-copyStart), last, end());
                }
                else {
                    std::ranges::move(first, last, mutPos);
                }
            }
            else {
                // move new elements
                std::uninitialized_move(first, last, mutPos);
            }
            
            m_size += copyCount;
            m_tail = (m_head + m_size) % capacity();
            
            return mutPos;
        }
        else {
            return end();
        }
    }

    Iterator erase(size_t index) override
    {
        if (index >= m_size) throw std::out_of_range("index exceeds vector size");
        std::destroy_at(&(*this)[index]);
        std::ranges::move(*this | std::views::drop(index+1), begin()+index);
        
        m_size--;
        m_tail = (m_head + m_size) % capacity();

        auto it = begin();
        std::advance(it, index);
        return it;
    }

    Iterator erase(size_t index, size_t count) override
    {
        if (index >= m_size) throw std::out_of_range("index exceeds vector size");
        std::ranges::destroy(*this | std::views::drop(index) | std::views::take(count));
        std::ranges::move(*this | std::views::drop(index+count), begin()+index);

        m_size -= count;
        m_tail = (m_head + m_size) % capacity();

        auto it = begin();
        std::advance(it, index);
        return it;
    }

    Iterator erase(ConstIterator pos) override
    {
        const auto index = std::distance(ConstIterator(begin()), pos);
        return erase(index);
    }

    Iterator erase(ConstIterator first, ConstIterator last) override
    {
        const auto index = std::distance(ConstIterator(begin()), first);
        const auto count = std::distance(first, last);
        return erase(index, count);
    }

    template<class... Args>
    OptionalRef emplace(Args&&... args)
    {
        if (full()) return std::nullopt;
        auto constructed = std::construct_at(&data()[m_tail], std::forward<Args>(args)...);
        m_tail = (m_tail + 1) % capacity();
        m_size++;
        return std::ref(*constructed);
    }

protected:
    StaticQueueBase() noexcept 
        : m_size{ 0 } , m_head{ 0 }, m_tail{ 0 }
    {}

private:
    constexpr ValueType* data() noexcept { return reinterpret_cast<ValueType*>(getStore().data()); }
    constexpr const ValueType* data() const noexcept { return reinterpret_cast<const ValueType*>(getStore().data()); }

    virtual StoreType& getStore() = 0;
    virtual const StoreType& getStore() const = 0;

protected:
    // ----------------------------------------
    // --- data
    // ----------------------------------------
    size_t m_size;
    size_t m_head;
    size_t m_tail;
};

template<typename T, size_t N, bool ClearOnDestroy = true>
requires std::default_initializable<T>
class StaticQueue : public StaticQueueBase<T>
{
    using Base      = StaticQueueBase<T>;
    using ValueType = Base::ValueType;
    using StoreType = Base::StoreType;

public:
    StaticQueue() noexcept 
        : Base(0, 0, 0)
    {}

    StaticQueue(const size_t size) 
        : Base(size, 0, size % N)
    { 
        if (size > N) throw std::length_error("Initial size exceeds static capacity"); 
        m_store.construct(0, size);
    }

    StaticQueue(const size_t size, const ValueType& value) 
        : Base(size, 0, size % N)
    {
        if (size > N) throw std::length_error("Initial size exceeds static capacity");
        if constexpr (std::is_copy_constructible_v<ValueType>) {
            m_store.uninitializedFill(0, size, value);
        }
        else {
            throw std::logic_error("copying not allowed");
        }
    }

    StaticQueue(const StaticQueue& other) 
        : Base(other.m_size, 0, other.m_size % N)
    {
        if constexpr (std::is_copy_constructible_v<ValueType>) {
            std::uninitialized_copy(other.begin(), other.end(), Base::begin());
        }
        else {
            throw std::logic_error("copying not allowed");
        }
    }

    StaticQueue(StaticQueue&& other) noexcept 
        : Base(other.m_size, 0, other.m_size % N)
    {
        if constexpr (std::is_move_constructible_v<ValueType>) {
            std::uninitialized_move(other.begin(), other.end(), Base::begin());
        }
        else {
            throw std::logic_error("moving not allowed");
        }
        other.clear();
    }

    StaticQueue(const std::initializer_list<ValueType>& data) 
        : Base(data.size(), 0, data.size() % N)
    {
        if (data.size() > N) throw std::length_error("Initial size exceeds static capacity");
        if constexpr (std::is_move_constructible_v<ValueType>) {
            std::uninitialized_move(data.begin(), data.end(), Base::begin());
        }
        else if constexpr (std::is_copy_constructible_v<ValueType>) {
            std::uninitialized_copy(data.begin(), data.end(), Base::begin());
        }
        else {
            throw std::logic_error("moving and copying not allowed");
        }
    }

    ~StaticQueue() 
    {
        Base::clear();
    }

    StaticQueue& operator=(const StaticQueue& other)
    {
        if constexpr (std::is_copy_assignable_v<ValueType>) {
            if (this != &other) {
                Base::clear();

                Base::m_size = other.m_size;
                Base::m_tail = other.m_size % N;

                std::uninitialized_copy(other.begin(), other.end(), Base::begin());
            }
            return *this;
        }
        else {
            throw std::logic_error("copying not allowed");
        }
    }

    StaticQueue& operator=(StaticQueue&& other) noexcept
    {
        if constexpr (std::is_move_assignable_v<ValueType>) {
            if (this != &other) {
                Base::clear();

                Base::m_size = other.m_size;
                Base::m_tail = other.m_size % N;

                std::uninitialized_move(other.begin(), other.end(), Base::begin());
                other.clear();
            }
            return *this;
        }
        else {
            throw std::logic_error("moving not allowed");
        }
    }

private:
    StoreType& getStore() override { return m_store; }
    const StoreType& getStore() const override { return m_store; }

    StaticObjectStore<ValueType, N, ClearOnDestroy> m_store;
};

static_assert(std::ranges::random_access_range<StaticQueue<int, 5>>);
static_assert(std::ranges::random_access_range<const StaticQueue<int, 5>>);

template<typename T>
requires std::default_initializable<T>
class StaticQueueView : public StaticQueueBase<T>
{
    using Base      = StaticQueueBase<T>;
    using ValueType = Base::ValueType;
    using StoreType = Base::StoreType;

public:
    StaticQueueView(StoreType& store) noexcept 
        : Base(0, 0, 0), m_store(store)
    {}

    StaticQueueView(const StaticQueueView& other) = delete;
    StaticQueueView(StaticQueueView&& other) = delete; 

    ~StaticQueueView() 
    {
        Base::clear();
    }

    StaticQueueView& operator=(const StaticQueueView& other) = delete;
    StaticQueueView& operator=(StaticQueueView&& other) = delete;

private:
    StoreType& getStore() override { return m_store; }
    const StoreType& getStore() const override { return m_store; }

    StoreType& m_store;
};

static_assert(std::ranges::random_access_range<StaticQueueView<int>>);
static_assert(std::ranges::random_access_range<const StaticQueueView<int>>);