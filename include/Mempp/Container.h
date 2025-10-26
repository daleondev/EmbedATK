#pragma once

#include "Memory.h"
#include "Polymorphic.h"

//------------------------------------------------------
//                      Iterators
//------------------------------------------------------

// Containers
#include <tuple>
#include <vector>
#include <list>
#include <deque>
#include <string>
#include <array>
#include <forward_list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <span>
#include <iterator>
#include <filesystem>
#include <regex>

using AllIterators = std::tuple<
    // --- Container Iterators ---
    std::vector<int>::iterator,
    std::list<int>::iterator,
    std::deque<int>::iterator,
    std::string::iterator,
    std::array<int, 1>::iterator,
    std::forward_list<int>::iterator,
    std::set<int>::iterator,
    std::map<int, int>::iterator,
    std::multiset<int>::iterator,
    std::multimap<int, int>::iterator,
    std::unordered_set<int>::iterator,
    std::unordered_map<int, int>::iterator,
    std::unordered_multiset<int>::iterator,
    std::unordered_multimap<int, int>::iterator,
    std::span<int>::iterator,
    
    // --- Iterator Adaptors ---
    std::reverse_iterator<std::vector<int>::iterator>,
    std::move_iterator<std::vector<int>::iterator>,
    std::common_iterator<std::counted_iterator<std::vector<int>::iterator>, std::default_sentinel_t>,
    
    // --- Stream Iterators ---
    std::istream_iterator<int>,
    std::ostream_iterator<int>,
    std::istreambuf_iterator<char>,
    std::ostreambuf_iterator<char>,

    // --- Specialized Iterators ---
    std::filesystem::directory_iterator,
    std::filesystem::recursive_directory_iterator,
    std::regex_iterator<std::string::const_iterator>,
    std::regex_token_iterator<std::string::const_iterator>
>;

#include <cstring>

template <typename T>
class AnyIterator;

template<typename IterT>
concept IsIterator = (!std::is_same_v<std::decay_t<IterT>, AnyIterator<IterT>>) && 
                        std::input_or_output_iterator<IterT>;

template <typename T>
class AnyIterator
{
    template <typename T_Friend>
    friend class AnyIterator;

public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using ValueType = T;

    // ----------------------------------------
    // --- constructors/detructors
    // ----------------------------------------
    template <IsIterator IterT>
    explicit AnyIterator(IterT iter)
    {
        m_self.template construct<Model<IterT>>(std::move(iter));
    }

    AnyIterator(const AnyIterator& other)
    {
        other.m_self.get()->cloneTo(m_self);
    }

    template<typename OtherT>
    AnyIterator(const AnyIterator<OtherT>& other) 
    requires(std::is_const_v<T> && std::is_same_v<T, const OtherT>)
    {
        other.getConcept()->cloneToConst(m_self);
    }

    AnyIterator(AnyIterator&& other) noexcept
    {
        other.m_self.get()->transferTo(m_self);
        other.m_self.destroy();
    }

    ~AnyIterator() = default;

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    friend bool operator==(const AnyIterator& a, const AnyIterator& b)
    {
        if (!a.m_self && !b.m_self) return true;
        if (!a.m_self || !b.m_self) return false;
        return a.m_self.get()->isEqual(b.m_self.get());
    }

    friend bool operator!=(const AnyIterator& a, const AnyIterator& b) 
    {
        return !(a == b);
    }

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    ValueType& operator*() const 
    {
        return m_self.get()->dereferenced();
    }

    template<IsIterator IterT>
    operator IterT()
    {
        auto* model = m_self.template as<Model<IterT>>();
        return model->iter;
    }

    // ----------------------------------------
    // --- modification
    // ----------------------------------------
    AnyIterator& operator=(const AnyIterator& other) 
    {
        if (this != &other) {
            other.m_self.get()->cloneTo(m_self);
        }
        return *this;
    }

    AnyIterator& operator=(AnyIterator&& other) noexcept
    {
        if (this != &other) {
            other.m_self.get()->transferTo(m_self);
            other.m_self.destroy();
        }
        return *this;
    }

    AnyIterator& operator++() 
    {
        m_self.get()->increment();
        return *this;
    }

    AnyIterator operator++(int) 
    {
        AnyIterator tmp = *this;
        m_self.get()->increment();
        return tmp;
    }

private:
    struct Concept 
    {
        // ----------------------------------------
        // --- constructors/detructors
        // ----------------------------------------
        virtual ~Concept() = default;

        // ----------------------------------------
        // --- information
        // ----------------------------------------
        virtual bool isEqual(const Concept* other) const = 0;
        virtual const std::type_info& type() const noexcept = 0;
        virtual ValueType& dereferenced() const = 0;

        // ----------------------------------------
        // --- modification
        // ----------------------------------------
        virtual void cloneTo(IPolymorphic<Concept>& other) const = 0;
        virtual void cloneToConst(IPolymorphic<typename AnyIterator<const T>::Concept>& other) const = 0;
        virtual void transferTo(IPolymorphic<Concept>& other) const = 0;
        virtual void increment() = 0;
    };

    template <typename IterT>
    struct Model final : public Concept 
    {
        // ----------------------------------------
        // --- constructors/detructors
        // ----------------------------------------
        Model(IterT iter) : iter(std::move(iter)) {}
        ~Model() = default;

        // ----------------------------------------
        // --- information
        // ----------------------------------------
        bool isEqual(const Concept* other) const override 
        {
            if (!other || type() != other->type()) {
                return false; 
            }
            return iter == static_cast<const Model<IterT>*>(other)->iter;
        }

        const std::type_info& type() const noexcept override 
        {
            return typeid(IterT);
        }

        // ----------------------------------------
        // --- data access
        // ----------------------------------------
        ValueType& dereferenced() const override { return *iter; }

        // ----------------------------------------
        // --- modification
        // ----------------------------------------
        void cloneTo(IPolymorphic<Concept>& other) const override 
        {
            other.template construct<Model<IterT>>(iter);
        }

        void cloneToConst(IPolymorphic<typename AnyIterator<const T>::Concept>& other) const override
        {
            if constexpr (std::is_const_v<T>) {
                cloneTo(other);
            }
            else {
                other.template construct<typename AnyIterator<const T>::template Model<IterT>>(iter);
            }
        }

        void transferTo(IPolymorphic<Concept>& other) const override 
        {
            other.template construct<Model<IterT>>(std::move(iter));
        }
        
        void increment() override { ++iter; }

        // ----------------------------------------
        // --- data
        // ----------------------------------------
        IterT iter;
    };

    const Concept* getConcept() const { return m_self.get(); }

    // ----------------------------------------
    // --- data
    // ----------------------------------------
    StaticPolymorphicStore<Concept, MaxAllocData<AllIterators>::data> m_self;
};

//------------------------------------------------------
//                      Iterable
//------------------------------------------------------

template<typename T>
class IIterable
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using ValueType     = T;
    using Iterator      = AnyIterator<T>;
    using ConstIterator = AnyIterator<const T>;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    ~IIterable() = default;

    // ----------------------------------------
    // --- iterators
    // ----------------------------------------
    virtual Iterator begin() = 0;
    virtual ConstIterator begin() const = 0;
    virtual Iterator end() = 0;
    virtual ConstIterator end() const = 0;

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    virtual bool empty() const noexcept = 0;
    virtual std::size_t size() const noexcept = 0;

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    virtual T* data() = 0;
    virtual const T* data() const = 0;
    virtual T& operator[](const std::size_t i) = 0;
    virtual const T& operator[](const std::size_t i) const = 0;
};

//------------------------------------------------------
//                      Vector
//------------------------------------------------------

template<typename T>
class IVector : public IIterable<T>
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Iterable      = IIterable<T>;
    using ValueType     = Iterable::ValueType;
    using Iterator      = Iterable::Iterator;
    using ConstIterator = Iterable::ConstIterator;

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
    virtual constexpr T& at(size_t index) = 0;
    virtual constexpr const T& at(size_t index) const = 0;

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
        m_vector.reserve(N); 
        m_vector.resize(size);
    }
    StaticStdVector(size_t size, const ValueType& val) 
        : m_pool{}, m_vector{&m_pool}
    { 
        m_vector.reserve(N); 
        m_vector.resize(size, val);
    }

    StaticStdVector(const StaticStdVector& other) 
        : m_pool{}, m_vector{&m_pool}
    {
        m_vector = other.m_vector;
    }

    StaticStdVector& operator=(const StaticStdVector& other)
    {
        if (this != &other) {
            m_vector = other.m_vector;
        }
        return *this;
    }

    StaticStdVector(StaticStdVector&& other) noexcept 
        : m_pool{}, m_vector{&m_pool}
    {
        m_vector = std::move(other.m_vector);
    }

    StaticStdVector& operator=(StaticStdVector&& other) noexcept
    {
        if (this != &other) {
            m_vector = std::move(other.m_vector);
        }
        return *this;
    }

    ~StaticStdVector() = default;

    // ----------------------------------------
    // --- iterators
    // ----------------------------------------
    Iterator begin() noexcept override { return Iterator(m_vector.begin()); }
    ConstIterator begin() const noexcept override { return ConstIterator(m_vector.cbegin()); }
    Iterator end() noexcept override { return Iterator(m_vector.end()); }
    ConstIterator end() const noexcept override { return ConstIterator(m_vector.cend()); }

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    bool empty() const noexcept override { return m_vector.empty(); }
    size_t size() const noexcept override { return m_vector.size(); }
    bool full() const noexcept override { return m_vector.size() == m_vector.capacity(); }
    size_t capacity() const noexcept override { return m_vector.capacity(); }
    
    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    ValueType* data() override { return m_vector.data(); }
    const ValueType* data() const override { return m_vector.data(); }
    ValueType& operator[](const size_t i) noexcept override { return m_vector[i]; }
    const ValueType& operator[](const size_t i) const noexcept override { return m_vector[i]; }
    ValueType& at(size_t i) override { return m_vector.at(i); }
    const ValueType& at(size_t i) const override { return m_vector.at(i); }
    
    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    void clear() noexcept override { m_vector.clear(); }
    void resize(size_t size) override { m_vector.resize(size); }
    void resize(size_t size, const ValueType& val) override { m_vector.resize(size, val); }
    void push_back(ValueType&& value) override { m_vector.push_back(std::move(value)); }
    void push_back(const ValueType& value) override { m_vector.push_back(value); }
    Iterator erase(size_t index) override 
    {
        auto it = m_vector.begin();
        std::advance(it, index);
        return Iterator(m_vector.erase(it));
    }
    Iterator erase(size_t index, size_t count) override 
    {
        auto it = m_vector.begin();
        std::advance(it, index);
        return Iterator(m_vector.erase(it, it+count));
    }
    Iterator erase(ConstIterator pos) override 
    {
        return Iterator(m_vector.erase(pos));
    }
    Iterator erase(ConstIterator first, ConstIterator last) override 
    {
        return Iterator(m_vector.erase(first, last));
    }

    template<class... Args>
    ValueType& emplace_back(Args&&... args) { return m_vector.emplace_back(std::forward<Args>(args)...); }

private:
    // ----------------------------------------
    // --- data
    // ----------------------------------------
    StaticEntiredPool<N, allocData<ValueType>()> m_pool;
    std::pmr::vector<ValueType> m_vector;
};