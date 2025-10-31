#pragma once

#include "Memory.h"
#include "Polymorphic.h"

//------------------------------------------------------
//                      Iterators
//------------------------------------------------------

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

template <typename T, typename Category>
class AnyIterator;

template <typename T>
struct is_any_iterator : std::false_type {};

template <typename T, typename C>
struct is_any_iterator<AnyIterator<T, C>> : std::true_type {};

template<typename IterT>
concept IsIterator = (!is_any_iterator<std::decay_t<IterT>>::value) && 
                        std::input_or_output_iterator<IterT>;

template <typename T, typename Category>
class AnyIterator
{
    template <typename T_Friend, typename C_Friend>
    friend class AnyIterator;

public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using value_type = T;
    using reference = T&;
    using pointer = T*;
    using difference_type = std::ptrdiff_t;
    using iterator_category = Category;

    // ----------------------------------------
    // --- constructors/detructors
    // ----------------------------------------
    AnyIterator() = default;

    template <IsIterator IterT>
    explicit AnyIterator(IterT iter)
    {
        using IterCategory = typename std::iterator_traits<IterT>::iterator_category;
        static_assert(std::is_convertible_v<IterCategory, Category>,
            "The provided iterator category is not compatible with the AnyIterator's category.");

        m_self.template construct<Model<IterT>>(std::move(iter));
    }

    AnyIterator(const AnyIterator& other)
    {
        other.m_self.get()->cloneTo(m_self);
    }

    template<typename OtherT>
    AnyIterator(const AnyIterator<OtherT, Category>& other) 
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
        if (!a.m_self.get() && !b.m_self.get()) return true;
        if (!a.m_self.get() || !b.m_self.get()) return false;
        return a.m_self.get()->isEqual(b.m_self.get());
    }

    friend bool operator!=(const AnyIterator& a, const AnyIterator& b) 
    {
        return !(a == b);
    }

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    reference operator*() const 
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
    requires std::is_convertible_v<iterator_category, std::forward_iterator_tag>
    {
        m_self.get()->increment();
        return *this;
    }

    AnyIterator operator++(int) 
    requires std::is_convertible_v<iterator_category, std::forward_iterator_tag>
    {
        AnyIterator tmp = *this;
        m_self.get()->increment();
        return tmp;
    }

    AnyIterator& operator--() 
    requires std::is_convertible_v<iterator_category, std::bidirectional_iterator_tag>
    {
        m_self.get()->decrement();
        return *this;
    }

    AnyIterator operator--(int) 
    requires std::is_convertible_v<iterator_category, std::bidirectional_iterator_tag>
    {
        AnyIterator tmp = *this;
        m_self.get()->decrement();
        return tmp;
    }

    AnyIterator operator+(size_t n)
    requires std::is_convertible_v<iterator_category, std::random_access_iterator_tag>
    {
        m_self.get()->increment(n);
        return *this;
    }

    AnyIterator& operator+=(size_t n)
    requires std::is_convertible_v<iterator_category, std::random_access_iterator_tag>
    {
        m_self.get()->increment(n);
        return *this;
    }

    AnyIterator operator-(size_t n)
    requires std::is_convertible_v<iterator_category, std::random_access_iterator_tag>
    {
        m_self.get()->decrement(n);
        return *this;
    }

    AnyIterator& operator-=(size_t n)
    requires std::is_convertible_v<iterator_category, std::random_access_iterator_tag>
    {
        m_self.get()->decrement(n);
        return *this;
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
        virtual value_type& dereferenced() const = 0;

        // ----------------------------------------
        // --- modification
        // ----------------------------------------
        virtual void cloneTo(IPolymorphic<Concept>& other) const = 0;
        virtual void cloneToConst(IPolymorphic<typename AnyIterator<const T, Category>::Concept>& other) const = 0;
        virtual void transferTo(IPolymorphic<Concept>& other) const = 0;
        virtual void increment() = 0;
        virtual void decrement() = 0;
        virtual void increment(size_t n) = 0;
        virtual void decrement(size_t n) = 0;
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
        value_type& dereferenced() const override { return *iter; }

        // ----------------------------------------
        // --- modification
        // ----------------------------------------
        void cloneTo(IPolymorphic<Concept>& other) const override 
        {
            other.template construct<Model<IterT>>(iter);
        }

        void cloneToConst(IPolymorphic<typename AnyIterator<const T, Category>::Concept>& other) const override
        {
            if constexpr (std::is_const_v<T>) {
                cloneTo(other);
            }
            else {
                other.template construct<typename AnyIterator<const T, Category>::template Model<IterT>>(iter);
            }
        }

        void transferTo(IPolymorphic<Concept>& other) const override 
        {
            other.template construct<Model<IterT>>(std::move(iter));
        }
        
        void increment() override { ++iter; }
        void decrement() override { --iter; }
        void increment(size_t n) override { iter += n; }
        void decrement(size_t n) override { iter -= n; }

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

template<typename T, typename IteratorCategory>
class IIterable
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using ValueType     = T;
    using Iterator      = AnyIterator<T, IteratorCategory>;
    using ConstIterator = AnyIterator<const T, IteratorCategory>;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    virtual ~IIterable() = default;

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
};

//------------------------------------------------------
//                      Container
//------------------------------------------------------

template<typename T, typename IteratorCategory = std::bidirectional_iterator_tag>
class IContigousContainer : public IIterable<T, IteratorCategory>
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Iterable      = IIterable<T, IteratorCategory>;
    using ValueType     = Iterable::ValueType;
    using Iterator      = Iterable::Iterator;
    using ConstIterator = Iterable::ConstIterator;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    virtual ~IContigousContainer() = default;

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    virtual T* data() = 0;
    virtual const T* data() const = 0;
    virtual T& operator[](const std::size_t i) = 0;
    virtual const T& operator[](const std::size_t i) const = 0;
    virtual constexpr T& at(size_t index) = 0;
    virtual constexpr const T& at(size_t index) const = 0;
};

template<typename K, typename V, typename IteratorCategory = std::bidirectional_iterator_tag>
class IAssociativeContainer : public IIterable<std::pair<K, V>, IteratorCategory>
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Iterable      = IIterable<std::pair<K, V>, IteratorCategory>;
    using KeyType       = K;
    using ValueType     = V;
    using Iterator      = Iterable::Iterator;
    using ConstIterator = Iterable::ConstIterator;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    virtual ~IAssociativeContainer() = default;

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    virtual ValueType& operator[](const KeyType& key) = 0;
    virtual const ValueType& operator[](const KeyType& key) const = 0;
    virtual ValueType& at(const KeyType& key) = 0;
    virtual const ValueType& at(const KeyType& key) const = 0;
};

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
    Iterator begin() override { return Iterator(m_vector.begin()); }
    ConstIterator begin() const override { return ConstIterator(m_vector.cbegin()); }
    Iterator end() override { return Iterator(m_vector.end()); }
    ConstIterator end() const override { return ConstIterator(m_vector.cend()); }

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
        return Iterator(m_vector.erase(m_vector.begin()+index));
    }
    Iterator erase(size_t index, size_t count) override 
    {
        return Iterator(m_vector.erase(m_vector.begin()+index, m_vector.begin()+index+count));
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

static_assert(std::ranges::bidirectional_range<StaticStdVector<int, 5>>);
static_assert(std::ranges::bidirectional_range<const StaticStdVector<int, 5>>);

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
    using InternalIterator      = std::span<T>::iterator;
    using InternalConstIterator = std::span<const T>::iterator;

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
        const auto index = std::distance(static_cast<InternalConstIterator>(begin()), static_cast<InternalConstIterator>(pos));
        return erase(index);
    }

    Iterator erase(ConstIterator first, ConstIterator last) override
    {
        const auto index = std::distance(static_cast<InternalConstIterator>(begin()), static_cast<InternalConstIterator>(first));
        const auto count = std::distance(static_cast<InternalConstIterator>(first), static_cast<InternalConstIterator>(last));
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

static_assert(std::ranges::bidirectional_range<StaticVector<int, 5>>);
static_assert(std::ranges::bidirectional_range<const StaticVector<int, 5>>);

//------------------------------------------------------
//                      Map
//------------------------------------------------------

template<typename K, typename V>
class IMap : IAssociativeContainer<K, V, std::bidirectional_iterator_tag>
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Container     = IAssociativeContainer<K, V, std::bidirectional_iterator_tag>;
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
    virtual bool full() const = 0;
    virtual size_t capacity() const = 0;

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
    Iterator begin() noexcept override { return m_map.begin(); }
    ConstIterator begin() const noexcept override { return m_map.begin(); }
    Iterator end() noexcept override { return m_map.end(); }
    ConstIterator end() const noexcept override { return m_map.end(); }

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    bool empty() const override { return m_map.empty(); }
    size_t size() const override { return m_map.size(); }
    size_t max_size() const override { return N; }

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    ValueType& operator[](const KeyType& key) override { return m_map[key]; }
    const ValueType& operator[](const KeyType& key) const override { return m_map[key]; }
    ValueType& at(const KeyType& key) override { return m_map.at(key); }
    const ValueType& at(const KeyType& key) const override { return m_map.at(key); }

    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    void clear() noexcept override { m_map.clear(); }
    std::pair<Iterator, bool> insert(const std::pair<const KeyType, ValueType>& node) override { return m_map.insert(node); }
    size_t erase(const KeyType& key) override { return m_map.erase(key); }

    template<class... Args>
    std::pair<Iterator, bool> emplace(Args&&... args) { return m_map.emplace(std::forward<Args>(args)...); }

private:
    StaticBlockPool<N, allocData<typename Handle::NodeEstimate>()> m_pool;
    std::pmr::map<KeyType, ValueType> m_map;
};