#pragma once

#include "Memory.h"
#include "Polymorphic.h"

//------------------------------------------------------
//                      Iterators
//------------------------------------------------------

using AllIterators = std::tuple<
    // --- Container Iterators ---
    std::vector<int>::iterator,
    std::vector<bool>::iterator,
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

template <typename T, typename Category>
class AnyIterator;

template <typename T>
struct is_any_iterator : std::false_type {};

template <typename T, typename C>
struct is_any_iterator<AnyIterator<T, C>> : std::true_type {};

template<typename IterT>
concept IsIterator = (!is_any_iterator<std::decay_t<IterT>>::value) && 
                        std::input_or_output_iterator<IterT>;

template <typename T, typename Category = std::bidirectional_iterator_tag>
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

    friend bool operator<(const AnyIterator& a, const AnyIterator& b)
    requires std::is_convertible_v<iterator_category, std::random_access_iterator_tag>
    {
        if (!a.m_self.get() || !b.m_self.get()) return false;
        return a.m_self.get()->isLess(b.m_self.get());
    }

    friend bool operator>(const AnyIterator& a, const AnyIterator& b)
    requires std::is_convertible_v<iterator_category, std::random_access_iterator_tag>
    {
        return b < a;
    }

    friend bool operator<=(const AnyIterator& a, const AnyIterator& b)
    requires std::is_convertible_v<iterator_category, std::random_access_iterator_tag>
    {
        return !(b < a);
    }

    friend bool operator>=(const AnyIterator& a, const AnyIterator& b)
    requires std::is_convertible_v<iterator_category, std::random_access_iterator_tag>
    {
        return !(a < b);
    }

    friend difference_type operator-(const AnyIterator& a, const AnyIterator& b)
    requires std::is_convertible_v<iterator_category, std::random_access_iterator_tag>
    {
        return a.m_self.get()->difference(b.m_self.get());
    }

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    reference operator*() const 
    {
        return m_self.get()->dereferenced();
    }

    reference operator[](difference_type n) const
    requires std::is_convertible_v<iterator_category, std::random_access_iterator_tag>
    {
        return m_self.get()->at(n);
    }

    // unsafe
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

    AnyIterator operator+(difference_type n) const
    requires std::is_convertible_v<iterator_category, std::random_access_iterator_tag>
    {
        AnyIterator tmp = *this;
        tmp += n;
        return tmp;
    }

    AnyIterator& operator+=(difference_type n)
    requires std::is_convertible_v<iterator_category, std::random_access_iterator_tag>
    {
        m_self.get()->increment(n);
        return *this;
    }

    AnyIterator operator-(difference_type n) const
    requires std::is_convertible_v<iterator_category, std::random_access_iterator_tag>
    {
        AnyIterator tmp = *this;
        tmp -= n;
        return tmp;
    }

    AnyIterator& operator-=(difference_type n)
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
        virtual difference_type difference(const Concept* other) const = 0;
        virtual bool isLess(const Concept* other) const = 0;

        // ----------------------------------------
        // --- data access
        // ----------------------------------------
        virtual value_type& dereferenced() const = 0;
        virtual reference at(difference_type n) const = 0;

        // ----------------------------------------
        // --- modification
        // ----------------------------------------
        virtual void cloneTo(IPolymorphic<Concept>& other) const = 0;
        virtual void cloneToConst(IPolymorphic<typename AnyIterator<const T, Category>::Concept>& other) const = 0;
        virtual void transferTo(IPolymorphic<Concept>& other) const = 0;
        virtual void increment() = 0;
        virtual void decrement() = 0;
        virtual void increment(difference_type n) = 0;
        virtual void decrement(difference_type n) = 0;
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

        difference_type difference(const Concept* other) const override
        {
            if (!other || type() != other->type()) {
                throw std::logic_error("iterators with different types are not comparable");
            }
            if constexpr (std::is_convertible_v<typename std::iterator_traits<IterT>::iterator_category, std::random_access_iterator_tag>)
                return iter - static_cast<const Model<IterT>*>(other)->iter;
            else
                throw std::logic_error("difference not supported for this iterator category");
        }

        bool isLess(const Concept* other) const override
        {
            if (!other || type() != other->type()) {
                return false;
            }
            if constexpr (std::is_convertible_v<typename std::iterator_traits<IterT>::iterator_category, std::random_access_iterator_tag>)
                return iter < static_cast<const Model<IterT>*>(other)->iter;
            else
                throw std::logic_error("isLess not supported for this iterator category");
        }

        // ----------------------------------------
        // --- data access
        // ----------------------------------------
        value_type& dereferenced() const override { return *iter; }

        reference at(difference_type n) const override
        {
            if constexpr (std::is_convertible_v<typename std::iterator_traits<IterT>::iterator_category, std::random_access_iterator_tag>)
                return iter[n];
            else
                throw std::logic_error("at() not supported for this iterator category");
        }

        // ----------------------------------------
        // --- modification
        // ----------------------------------------
        void cloneTo(IPolymorphic<Concept>& other) const override 
        {
            other.template construct<Model<IterT>>(iter);
        }

        void cloneToConst(IPolymorphic<typename AnyIterator<const T, Category>::Concept>& other) const override
        {
            other.template construct<typename AnyIterator<const T, Category>::template Model<IterT>>(iter);
        }

        void transferTo(IPolymorphic<Concept>& other) const override 
        {
            other.template construct<Model<IterT>>(std::move(iter));
        }
        
        void increment() override { ++iter; }
        void decrement() override { --iter; }
        void increment(difference_type n) override 
        {
            if constexpr (std::is_convertible_v<typename std::iterator_traits<IterT>::iterator_category, std::random_access_iterator_tag>)
                iter += n;
            else
                throw std::logic_error("increment(n) not supported for this iterator category");
        }
        void decrement(difference_type n) override 
        {
            if constexpr (std::is_convertible_v<typename std::iterator_traits<IterT>::iterator_category, std::random_access_iterator_tag>)
                iter -= n;
            else
                throw std::logic_error("decrement(n) not supported for this iterator category");
        }

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

template <typename T, typename Category>
AnyIterator<T, Category> operator+(typename AnyIterator<T, Category>::difference_type n, const AnyIterator<T, Category>& it)
    requires std::is_convertible_v<typename AnyIterator<T, Category>::iterator_category, std::random_access_iterator_tag>
{
    return it + n;
}

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

template<typename T>
class ISequentialContainer : public IIterable<T, std::random_access_iterator_tag>
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Iterable      = IIterable<T, std::random_access_iterator_tag>;
    using ValueType     = Iterable::ValueType;
    using Iterator      = Iterable::Iterator;
    using ConstIterator = Iterable::ConstIterator;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    virtual ~ISequentialContainer() = default;

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    virtual T& operator[](const std::size_t i) = 0;
    virtual const T& operator[](const std::size_t i) const = 0;
    virtual constexpr T& at(size_t index) = 0;
    virtual constexpr const T& at(size_t index) const = 0;
};

template<typename K, typename V>
class IAssociativeContainer : public IIterable<std::pair<const K, V>, std::bidirectional_iterator_tag>
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Iterable      = IIterable<std::pair<const K, V>, std::bidirectional_iterator_tag>;
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

template<typename T>
class IContigousContainer : public ISequentialContainer<T>
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Container     = ISequentialContainer<T>;
    using ValueType     = Container::ValueType;
    using Iterator      = Container::Iterator;
    using ConstIterator = Container::ConstIterator;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    virtual ~IContigousContainer() = default;

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    virtual T* data() = 0;
    virtual const T* data() const = 0;
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
    virtual Iterator insert(ConstIterator pos, std::move_iterator<ConstIterator> first, std::move_iterator<ConstIterator> last) = 0;
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

    virtual Iterator insert(ConstIterator pos, std::move_iterator<ConstIterator> first, std::move_iterator<ConstIterator> last) override 
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

template<typename T, size_t N, bool ClearOnDestroy = true>
requires std::default_initializable<T>
class StaticQueue : public IQueue<T>
{   
    static_assert(std::is_same<typename std::remove_cv<T>::type, T>::value,
	  "StaticQueue must have a non-const, non-volatile value_type");

    template<typename T_friend, size_t N_friend, bool ClearOnDestroy_Friend>
    requires std::default_initializable<T_friend>
    friend class StaticQueue;

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
        using owner_type        = std::conditional_t<IsConst, const StaticQueue, StaticQueue>;

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
    
    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    StaticQueue() noexcept 
        : m_store{}, m_size{ 0 } , m_head{ 0 }, m_tail{ 0 }
    {}

    StaticQueue(const size_t size) 
        : m_store{}, m_size{ size } , m_head{ 0 }, m_tail{ m_size % N }
    { 
        if (size > N) throw std::length_error("Initial size exceeds static capacity"); 
        m_store.construct(0, m_size);
    }

    StaticQueue(const size_t size, const ValueType& value) 
        : m_store{}, m_size{ size }, m_head{ 0 }, m_tail{ m_size % N }
    {
        if (size > N) throw std::length_error("Initial size exceeds static capacity");
        m_store.uninitializedFill(0, m_size, value);
    }

    StaticQueue(const StaticQueue& other) 
        : m_store{}, m_size{ other.m_size }, m_head{ 0 }, m_tail{ m_size % N }
    {
        std::uninitialized_copy(other.begin(), other.end(), begin());
    }

    StaticQueue(StaticQueue&& other) noexcept 
        : m_store{}, m_size{ other.m_size }, m_head{ 0 }, m_tail{ m_size % N }
    {
        std::uninitialized_move(other.begin(), other.end(), begin());
        other.clear();
    }

    StaticQueue(const std::initializer_list<ValueType>& data) 
        : m_store{}, m_size{ data.size() }, m_head{ 0 }, m_tail{ m_size % N}
    {
        if (data.size() > N) throw std::length_error("Initial size exceeds static capacity");
        std::uninitialized_move(data.begin(), data.end(), begin());
    }

    ~StaticQueue()
    {
        clear();
    }

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
    constexpr bool full() const noexcept override { return m_size >= N; }
    constexpr size_t size() const noexcept override { return m_size; }
    constexpr size_t capacity() const noexcept override { return N; }

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    ValueType& operator[](size_t index) override { return data()[(m_head + index) % N]; }
    const ValueType& operator[](size_t index) const override { return data()[(m_head + index) % N]; }

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
    StaticQueue& operator=(const StaticQueue& other)
    {
        if (this != &other) {
            clear();

            m_size = other.m_size;
            m_tail = other.m_size % N;

            std::uninitialized_copy(other.begin(), other.end(), begin());
        }
        return *this;
    }

    StaticQueue& operator=(StaticQueue&& other) noexcept
    {
        if (this != &other) {
            clear();

            m_size = other.m_size;
            m_tail = other.m_size % N;

            std::uninitialized_move(other.begin(), other.end(), begin());
            other.clear();
        }
        return *this;
    }

    void clear() noexcept override
    {
        if (m_head > m_tail) {
            m_store.destroy(m_tail, N-m_tail);
            m_store.destroy(0, m_head);
        } else {
            m_store.destroy(m_head, m_tail-m_head);
        }
        m_size = 0;
        m_head = 0;
        m_tail = 0;
    }

    void resize(size_t size) override
    {
        if (size > N) throw std::length_error("Initial size exceeds static capacity"); 
        if (size == m_size) return;

        auto tail = (m_head + size) % N;
        if (size > m_size) {
            if (m_tail > tail) {
                m_store.construct(m_tail, N-m_tail);
                m_store.construct(0, tail);
            } else {
                m_store.construct(m_tail, tail-m_tail);
            }
        }
        else {
            if (tail > m_tail) {
                m_store.destroy(tail, N-tail);
                m_store.destroy(0, m_tail);
            } else {
                m_store.destroy(tail, m_tail-tail);
            }
        }
        m_size = size;
        m_tail = tail;
    }

    void resize(size_t size, const ValueType& value) override
    {
        if (size > N) throw std::length_error("Initial size exceeds static capacity"); 
        if (size == m_size) return;

        auto tail = (m_head + size) % N;
        if (size > m_size) {
            if (m_tail > tail) {
                m_store.uninitializedFill(m_tail, N-m_tail, value);
                m_store.uninitializedFill(0, tail, value);
            } else {
                m_store.uninitializedFill(m_tail, tail-m_tail, value);
            }
        }
        else {
            if (tail > m_tail) {
                m_store.destroy(tail, N-tail);
                m_store.destroy(0, m_tail);
            } else {
                m_store.destroy(tail, m_tail-tail);
            }
        }
        m_size = size;
        m_tail = tail;
    }

    bool push(const ValueType& value) override
    {
        if (full()) return false;
        std::construct_at(&data()[m_tail], value);
        m_tail = (m_tail + 1) % N;
        m_size++;
        return true;
    }

    bool push(ValueType&& value) override
    {
        if (full()) return false;
        std::construct_at(&data()[m_tail], std::move(value));
        m_tail = (m_tail + 1) % N;
        m_size++;
        return true;
    }

    std::optional<ValueType> pop() override
    {
        if (empty()) return std::nullopt;
        ValueType value = std::move(data()[m_head]);
        m_store.destroy(m_head, 1);
        m_head = (m_head + 1) % N;
        m_size--;
        return std::move(value);
    }

    Iterator insert(ConstIterator pos, Iterator first, Iterator last) override
    {
        const auto copyStart = std::distance(ConstIterator(begin()), pos);
        const auto copyCount = std::distance(first, last);
        const auto copyEnd = copyStart + copyCount;

        auto mutPos = Iterator(begin()+copyStart);
        if (copyCount == 0) return mutPos;
        if (m_size + copyCount > N) throw std::length_error("Insert would exceed static queue capacity");

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
        m_tail = (m_head + m_size) % N;
        
        return mutPos;
    }

    Iterator insert(ConstIterator pos, std::move_iterator<ConstIterator> first, std::move_iterator<ConstIterator> last) override
    {
        const auto copyStart = std::distance(ConstIterator(begin()), pos);
        const auto copyCount = std::distance(first, last);
        const auto copyEnd = copyStart + copyCount;

        auto mutPos = Iterator(begin()+copyStart);
        if (copyCount == 0) return mutPos;
        if (m_size + copyCount > N) throw std::length_error("Insert would exceed static queue capacity");

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
            std::uninitialized_move(first, last, pos);
        }
        
        m_size += copyCount;
        m_tail = (m_head + m_size) % N;
        
        return mutPos;
    }

    Iterator erase(size_t index) override
    {
        if (index >= m_size) throw std::out_of_range("index exceeds vector size");
        std::destroy_at(&(*this)[index]);
        std::ranges::move(*this | std::views::drop(index+1), begin()+index);
        
        m_size--;
        m_tail = (m_head + m_size) % N;

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
        m_tail = (m_head + m_size) % N;

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
        m_tail = (m_tail + 1) % N;
        m_size++;
        return std::ref(*constructed);
    }

private:
    constexpr ValueType* data() noexcept { return reinterpret_cast<ValueType*>(m_store.data()); }
    constexpr const ValueType* data() const noexcept { return reinterpret_cast<const ValueType*>(m_store.data()); }

    // ----------------------------------------
    // --- data
    // ----------------------------------------
    StaticObjectStore<ValueType, N, ClearOnDestroy> m_store;
    size_t m_size;
    size_t m_head;
    size_t m_tail;
};

static_assert(std::ranges::random_access_range<StaticQueue<int, 5>>);
static_assert(std::ranges::random_access_range<const StaticQueue<int, 5>>);

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