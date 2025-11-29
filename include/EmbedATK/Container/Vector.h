#pragma once

#include "Container.h"

#include "EmbedATK/Memory/Pool.h"
#include "EmbedATK/Memory/Store.h"

//------------------------------------------------------
//                      Vector
//------------------------------------------------------

template<typename T>
class IVector : public IContigousContainer<T>
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Container     = IContigousContainer<T>;
    using ValueType     = Container::ValueType;
    using Iterator      = Container::Iterator;
    using ConstIterator = Container::ConstIterator;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    virtual ~IVector() = default;

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    virtual constexpr bool full() const noexcept = 0;
    virtual constexpr size_t capacity() const noexcept = 0;

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    virtual constexpr ValueType& front() = 0;
    virtual constexpr const ValueType& front() const = 0;
    virtual constexpr ValueType& back() = 0;
    virtual constexpr const ValueType& back() const = 0;

    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    virtual void clear() noexcept = 0;
    virtual void resize(const size_t size) = 0;
    virtual void resize(const size_t size, const ValueType& value) = 0;
    virtual void push_back(const ValueType& value) = 0;
    virtual void push_back(ValueType&& value) = 0;
    virtual Iterator erase(size_t index) = 0;
    virtual Iterator erase(size_t index, size_t count) = 0;
    virtual Iterator erase(ConstIterator pos) = 0;
    virtual Iterator erase(ConstIterator first, ConstIterator last) = 0;
};

template<typename T, size_t N>
class StaticStdVector : public IVector<T>
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Handle        = IVector<T>;
    using ValueType     = Handle::ValueType;
    using Iterator      = Handle::Iterator;
    using ConstIterator = Handle::ConstIterator;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    StaticStdVector() 
        : m_pool{}, m_vector{&m_pool}
    {       
        m_vector.reserve(N); 
    }
    StaticStdVector(size_t size)
        : m_pool{}, m_vector{&m_pool}
    { 
        if (size > N) throw std::length_error("Initial size exceeds static capacity");
        m_vector.reserve(N); 
        m_vector.resize(size);
    }
    StaticStdVector(size_t size, const ValueType& val) 
        : m_pool{}, m_vector{&m_pool}
    { 
        if (size > N) throw std::length_error("Initial size exceeds static capacity");
        m_vector.reserve(N); 
        m_vector.resize(size, val);
    }

    StaticStdVector(const StaticStdVector& other) 
        : m_pool{}, m_vector{&m_pool}
    {
        m_vector = other.m_vector;
    }

    StaticStdVector(StaticStdVector&& other) noexcept 
        : m_pool{}, m_vector{&m_pool}
    {
        m_vector = std::move(other.m_vector);
    }

    StaticStdVector(const std::initializer_list<ValueType>& data) 
        : m_pool{}, m_vector{data, &m_pool}
    {
        if (data.size() > N) throw std::length_error("Initial size exceeds static capacity");
    }

    ~StaticStdVector() = default;

    // ----------------------------------------
    // --- iterators
    // ----------------------------------------
    Iterator begin() override { return Iterator(m_vector.begin()); }
    ConstIterator begin() const override { return ConstIterator(m_vector.cbegin()); }
    Iterator end() override { return Iterator(m_vector.end()); }
    ConstIterator end() const override { return ConstIterator(m_vector.cend()); }

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    bool empty() const noexcept override { return m_vector.empty(); }
    bool full() const noexcept override { return m_vector.size() == N; }
    size_t size() const noexcept override { return m_vector.size(); }
    size_t capacity() const noexcept override { return N; }
    
    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    ValueType* data() override { return m_vector.data(); }
    const ValueType* data() const override { return m_vector.data(); }
    ValueType& operator[](const size_t i) noexcept override { return m_vector[i]; }
    const ValueType& operator[](const size_t i) const noexcept override { return m_vector[i]; }
    ValueType& at(size_t i) override { return m_vector.at(i); }
    const ValueType& at(size_t i) const override { return m_vector.at(i); }
    ValueType& front() override { return m_vector.front(); }
    const ValueType& front() const override { return m_vector.front(); }
    ValueType& back() override { return m_vector.back(); }
    const ValueType& back() const override { return m_vector.back(); }
    
    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    StaticStdVector& operator=(const StaticStdVector& other)
    {
        if (this != &other) {
            m_vector = other.m_vector;
        }
        return *this;
    }

    StaticStdVector& operator=(StaticStdVector&& other) noexcept
    {
        if (this != &other) {
            m_vector = std::move(other.m_vector);
        }
        return *this;
    }

    void clear() noexcept override { m_vector.clear(); }

    void resize(size_t size) override 
    { 
        if (size > N) throw std::length_error("vector exceeds static capacity");
        m_vector.resize(size); 
    }

    void resize(size_t size, const ValueType& val) override 
    { 
        if (size > N) throw std::length_error("vector exceeds static capacity");
        m_vector.resize(size, val); 
    }

    void push_back(ValueType&& value) override 
    { 
        if (full()) throw std::length_error("vector exceeds static capacity");
        m_vector.push_back(std::move(value)); 
    }

    void push_back(const ValueType& value) override 
    { 
        if (full()) throw std::length_error("vector exceeds static capacity");
        m_vector.push_back(value); 
    }

    Iterator erase(size_t index) override 
    {
        return Iterator(m_vector.erase(m_vector.begin()+index));
    }

    Iterator erase(size_t index, size_t count) override 
    {
        return Iterator(m_vector.erase(m_vector.begin()+index, m_vector.begin()+index+count));
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
    ValueType& emplace_back(Args&&... args) { return m_vector.emplace_back(std::forward<Args>(args)...); }

private:
    // ----------------------------------------
    // --- data
    // ----------------------------------------
    StaticEntiredPool<ValueType, N> m_pool;
    std::pmr::vector<ValueType> m_vector;
};

static_assert(std::ranges::random_access_range<StaticStdVector<int, 5>>);
static_assert(std::ranges::random_access_range<const StaticStdVector<int, 5>>);

template<typename T, size_t N, bool ClearOnDestroy = true>
requires std::default_initializable<T>
class StaticVector : public IVector<T>
{
    static_assert(std::is_same<typename std::remove_cv<T>::type, T>::value,
	  "StaticVector must have a non-const, non-volatile value_type");

public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Handle                = IVector<T>;
    using ValueType             = Handle::ValueType;
    using Iterator              = Handle::Iterator;
    using ConstIterator         = Handle::ConstIterator;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    StaticVector() noexcept : m_store{}, m_size{ 0 } {}

    StaticVector(const size_t size) : m_store{}, m_size{ size } 
    { 
        if (size > N) throw std::length_error("Initial size exceeds static capacity");
        m_store.construct(0, m_size);
    }

    StaticVector(const size_t size, const ValueType& value) : m_store{}, m_size{ size }
    {
        if (size > N) throw std::length_error("Initial size exceeds static capacity");
        m_store.uninitializedFill(0, m_size, value);
    }

    StaticVector(const StaticVector& other) : m_store{}, m_size{ other.m_size }
    {
        m_store.uninitializedClone(other.m_store, 0, m_size);
    }

    StaticVector(StaticVector&& other) noexcept : m_store{}, m_size{ other.m_size }
    {
        m_store.uninitializedTransfer(std::move(other.m_store), 0, m_size);
        other.clear();
    }

    StaticVector(const std::initializer_list<ValueType>& data) : m_store{}, m_size{ data.size() }
    {
        if (data.size() > N) throw std::length_error("Initial size exceeds static capacity");
        std::uninitialized_move(data.begin(), data.end(), begin());
    }

    ~StaticVector()
    {
        clear();
    }

    // ----------------------------------------
    // --- iterators
    // ----------------------------------------
    constexpr Iterator begin() override { return Iterator(std::span<T>{data(), m_size}.begin()); }
    constexpr ConstIterator begin() const override { return ConstIterator(std::span<const T>{data(), m_size}.begin()); }
    constexpr Iterator end() override { return Iterator(std::span<T>{data(), m_size}.end()); }
    constexpr ConstIterator end() const override { return ConstIterator(std::span<const T>{data(), m_size}.end()); }

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    constexpr bool empty() const noexcept override { return m_size == 0; }
    constexpr bool full() const noexcept override { return m_size >= N; }
    constexpr size_t size() const noexcept override { return m_size; }
    constexpr size_t capacity() const noexcept override { return N; }

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    constexpr ValueType* data() noexcept override { return reinterpret_cast<ValueType*>(m_store.data()); }
    constexpr const ValueType* data() const noexcept override { return reinterpret_cast<const ValueType*>(m_store.data()); }
    constexpr ValueType& operator[](const size_t index) noexcept override { return data()[index]; }
    constexpr const ValueType& operator[](const size_t index) const noexcept override { return data()[index]; }
    constexpr ValueType& front() override 
    {
        if (empty()) throw std::length_error("vector empty");
        return *begin(); 
    }
    constexpr const ValueType& front() const override
    {
        if (empty()) throw std::length_error("vector empty");
        return *begin(); 
    }
    constexpr ValueType& back() override 
    {
        if (empty()) throw std::length_error("vector empty");
        return *(end() - 1); 
    }
    constexpr const ValueType& back() const override 
    {
        if (empty()) throw std::length_error("vector empty");
        return *(end() - 1); 
    }

    constexpr ValueType& at(size_t index) override
    { 
        if (index >= m_size) throw std::out_of_range("index exceeds vector size");
        return data()[index]; 
    }

    constexpr const ValueType& at(size_t index) const  override
    { 
        if (index >= m_size) throw std::out_of_range("index exceeds vector size");
        return data()[index]; 
    }

    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    StaticVector& operator=(const StaticVector& other)
    {
        if (this != &other) {
            if (other.m_size > m_size) {
                m_store.clone(other.m_store, 0, m_size);
                m_store.uninitializedClone(other.m_store, m_size, other.m_size-m_size);
            }
            else {
                m_store.clone(other.m_store, 0, other.m_size);
                if (m_size > other.m_size) {
                    m_store.destroy(other.m_size, m_size-other.m_size);
                }
            }
            m_size = other.m_size;
        }
        return *this;
    }

    StaticVector& operator=(StaticVector&& other) noexcept
    {
        if (this != &other) {
            if (other.m_size > m_size) {
                m_store.transfer(std::move(other.m_store), 0, m_size);
                m_store.uninitializedTransfer(std::move(other.m_store), m_size, other.m_size-m_size);
            }
            else {
                m_store.transfer(std::move(other.m_store), 0, other.m_size);
                if (m_size > other.m_size) {
                    m_store.destroy(other.m_size, m_size-other.m_size);
                }
            }
            m_size = other.m_size;
            other.clear();
        }
        return *this;
    }

    void clear() noexcept override
    {
        m_store.destroy(0, m_size);
        m_size = 0;
    }

    void resize(const size_t size) override
    {
        if (size > N) throw std::length_error("vector exceeds static capacity");

        if (size > m_size) {
            m_store.construct(m_size, size-m_size);
        }
        else if (size < m_size) {
            m_store.destroy(size, m_size-size);
        }

        m_size = size;
    }

    void resize(const size_t size, const ValueType& value) override
    {
        if (size > N) throw std::length_error("vector exceeds static capacity");

        if (size > m_size) {
            m_store.uninitializedFill(m_size, size-m_size, value);
        }
        else if (size < m_size) {
            m_store.destroy(size, m_size-size);
        }

        m_size = size;
    }

    void push_back(const ValueType& value) override
    {
        if (full()) throw std::length_error("vector exceeds static capacity");
        std::construct_at(&data()[m_size], value);
        m_size++;
    }

    void push_back(ValueType&& value) override
    {
        if (full()) throw std::length_error("vector exceeds static capacity");
        std::construct_at(&data()[m_size], std::move(value));
        m_size++;
    }

    Iterator erase(size_t index) override
    {
        if (index >= m_size) throw std::out_of_range("index exceeds vector size");
        std::destroy_at(&data()[index]);
        std::ranges::move(*this | std::views::drop(index+1), data()+index);
        m_size--;
        auto it = begin();
        std::advance(it, index);
        return it;
    }

    Iterator erase(size_t index, size_t count) override
    {
        if (index >= m_size) throw std::out_of_range("index exceeds vector size");
        std::destroy_n(&data()[index], count);
        std::ranges::move(*this | std::views::drop(index+count), data()+index);
        m_size -= count;
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
    ValueType& emplace_back(Args&&... args)
    {
        if (full()) throw std::length_error("vector exceeds static capacity");
        std::construct_at(&data()[m_size], std::forward<Args>(args)...);
        return data()[m_size++];
    }

private:
    // ----------------------------------------
    // --- data
    // ----------------------------------------
    StaticObjectStore<ValueType, N, ClearOnDestroy> m_store;
    size_t m_size;
};

static_assert(std::ranges::random_access_range<StaticVector<int, 5>>);
static_assert(std::ranges::random_access_range<const StaticVector<int, 5>>);