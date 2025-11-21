#pragma once

#include "EmbedATK/Memory/Polymorphic.h"

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

    pointer operator->() 
    { 
        return &m_self.get()->dereferenced(); 
    }

    const pointer operator->() const 
    { 
        return &m_self.get()->dereferenced(); 
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
