#pragma once

#include "Container.h"

//------------------------------------------------------
//                      Map
//------------------------------------------------------

template<typename K, typename V>
class IMap : IAssociativeContainer<K, V>
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Container     = IAssociativeContainer<K, V>;
    using KeyType       = K;
    using ValueType     = V;
    using Iterator      = Container::Iterator;
    using ConstIterator = Container::ConstIterator;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    virtual ~IMap() = default;

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    virtual bool full() const noexcept = 0;
    virtual size_t capacity() const noexcept = 0;

    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    virtual void clear() noexcept = 0;
    virtual std::pair<Iterator, bool> insert(const std::pair<const KeyType, ValueType>& node) = 0;
    virtual size_t erase(const KeyType& key) = 0;

protected:
    struct NodeEstimate {
        std::pair<
            const KeyType, 
            ValueType
        > value;
        void* ptrs[3]; // parent, left, right
        char color;
    };
};

template<typename K, typename V, size_t N>
class StaticStdMap : public IMap<K ,V>
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Handle        = IMap<K ,V>;
    using KeyType       = Handle::KeyType;
    using ValueType     = Handle::ValueType;
    using Iterator      = Handle::Iterator;
    using ConstIterator = Handle::ConstIterator;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    StaticStdMap() 
        : m_pool{}, m_map{&m_pool}
    {}

    StaticStdMap(const StaticStdMap& other) 
        : m_pool{}, m_map{&m_pool}
    {
        m_map = other.m_map;
    }

    StaticStdMap& operator=(const StaticStdMap& other)
    {
        if (this == &other) return *this;
        m_map = other.m_map;
        return *this;
    }

    StaticStdMap(StaticStdMap&& other) noexcept 
        : m_pool{}, m_map{&m_pool}
    {
        m_map = std::move(other.m_map);
    }

    StaticStdMap& operator=(StaticStdMap&& other) noexcept
    {
        if (this == &other) return *this;
        m_map = std::move(other.m_map);
        return *this;
    }

    ~StaticStdMap() = default;

    // ----------------------------------------
    // --- iterators
    // ----------------------------------------
    Iterator begin() override { return Iterator(m_map.begin()); }
    ConstIterator begin() const override { return ConstIterator(m_map.cbegin()); }
    Iterator end() override { return Iterator(m_map.end()); }
    ConstIterator end() const override { return ConstIterator(m_map.cend()); }

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    bool empty() const noexcept override { return m_map.empty(); }
    bool full() const noexcept override { return m_map.size() >= N; };
    size_t size() const noexcept override { return m_map.size(); }
    size_t capacity() const noexcept override { return N; }

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    ValueType& operator[](const KeyType& key) override { return m_map[key]; }
    const ValueType& operator[](const KeyType& key) const override { return m_map.at(key); }
    ValueType& at(const KeyType& key) override { return m_map.at(key); }
    const ValueType& at(const KeyType& key) const override { return m_map.at(key); }

    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    void clear() noexcept override { m_map.clear(); }

    std::pair<Iterator, bool> insert(const std::pair<const KeyType, ValueType>& node) override
    {
        if (full()) throw std::length_error("map size exceeds static capacity");
        auto result = m_map.insert(node);
        return std::make_pair(Iterator(result.first), result.second);
    }

    size_t erase(const KeyType& key) override { return m_map.erase(key); }

    template<class... Args>
    std::pair<Iterator, bool> emplace(Args&&... args)
    {
        if (full()) throw std::length_error("map size exceeds static capacity");
        auto result = m_map.emplace(std::forward<Args>(args)...);
        return std::make_pair(Iterator(result.first), result.second);
    }

private:
    StaticBlockPool<N, allocData<typename Handle::NodeEstimate>()> m_pool;
    std::pmr::map<KeyType, ValueType> m_map;
};

static_assert(std::ranges::bidirectional_range<StaticStdMap<int, float, 5>>);
static_assert(std::ranges::bidirectional_range<const StaticStdMap<int, float, 5>>);