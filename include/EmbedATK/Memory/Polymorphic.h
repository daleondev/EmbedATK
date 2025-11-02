#pragma once

#include "Buffer.h"

#include "EmbedATK/Core/Concepts.h"

//------------------------------------------------------
//                   Polymorphic
//------------------------------------------------------

template<typename Base>
class IPolymorphic 
{
public:
    // ----------------------------------------
    // --- constructors/detructors
    // ----------------------------------------
    IPolymorphic(bool isStatic) : m_isStatic{isStatic} {}
    virtual ~IPolymorphic() = default;

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    const std::type_info& type() const { return *m_type; }
    operator bool() { return get() != nullptr; }
    operator bool() const { return get() != nullptr; }
    virtual constexpr size_t size() const = 0;

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    virtual Base* get() = 0;
    virtual const Base* get() const = 0;

    template<IsBaseOf<Base> Derived>
    Derived* as() { return static_cast<Derived*>(get()); }

    template<IsBaseOf<Base> Derived>
    const Derived* as() const { return static_cast<const Derived*>(get()); }

    template<class Derived>
    Derived* cast() 
    { 
        if (m_type && *m_type == typeid(Derived)) {
            return static_cast<Derived*>(get());
        }
        return nullptr;
    }

    template<class Derived>
    const Derived* cast() const 
    {
        if (m_type && *m_type == typeid(Derived)) {
            return static_cast<const Derived*>(get());
        }
        return nullptr;
    }

    // ----------------------------------------
    // --- modification
    // ----------------------------------------
    virtual void set(Base* data) = 0;

    template<IsBaseOf<Base> Derived, typename... Args>
    void construct(Args&&... args)
    {
        if (m_isStatic && get()) {
            std::destroy_at(get());
        }
        
        init();
        if (m_isStatic) {
            std::construct_at(static_cast<Derived*>(get()), std::forward<Args>(args)...);
        }
        else {
            set(new Derived(std::forward<Args>(args)...));
        }
        m_type = &typeid(Derived);
    }

    template<IsBaseOf<Base> Derived>
    void clone(const Derived& source)
    {
        construct<Derived>(source);
    }

    template<IsBaseOf<Base> Derived>
    void transfer(Derived&& source)
    {
        construct<Derived>(std::move(source));
    }

    void destroy()
    {
        if (m_isStatic) {
            if (get()) {
                std::destroy_at(get());
            }
        }

        reset();
        m_type = &typeid(void);
    }

protected:
    // ----------------------------------------
    // --- modification
    // ----------------------------------------
    virtual void init() = 0;
    virtual void reset() = 0;

    // ----------------------------------------
    // --- data
    // ----------------------------------------
    const std::type_info* m_type = &typeid(void);
    const bool m_isStatic;
};

//------------------------------------------------------
//                      Static
//------------------------------------------------------

template<typename Base, AllocData Alloc>
class StaticPolymorphicStore : public IPolymorphic<Base>
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Handle = IPolymorphic<Base>;

    // ----------------------------------------
    // --- constructors/detructors
    // ----------------------------------------
    StaticPolymorphicStore() : Handle(true) {}

    StaticPolymorphicStore(const StaticPolymorphicStore&) = delete; // copying raw data not safe
    StaticPolymorphicStore(StaticPolymorphicStore&& other) noexcept // moving raw data should be fine
        : Handle(true)
    {
        if (other.get()) {
            std::ranges::move(other.m_buff, m_buff.begin());
            init();

            other.m_isValid = false;
            other.reset();
        }
    }

    virtual ~StaticPolymorphicStore()
    {
        if (m_isValid) {
            Handle::destroy();
        }
    }

    template<IsBaseOf<Base> Derived>
    StaticPolymorphicStore(const Derived& other) // converting constructor instead of copy constructor
        : Handle(true)
    {
        if (m_isValid) {
            Handle::destroy();
        }

        if (other.get()) {
            Handle::clone(other);
            init();
        }
    }

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    constexpr size_t size() const override { return Alloc.size; }

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    Base* get() override { return m_data; }
    const Base* get() const override { return m_data; }
    operator Base&() { return *m_data; }
    operator const Base&() const { return *m_data; }

    // ----------------------------------------
    // --- modification
    // ----------------------------------------
    void set(Base*) override {}

    StaticPolymorphicStore& operator=(const StaticPolymorphicStore&) = delete; // copying raw data not safe
    StaticPolymorphicStore& operator=(StaticPolymorphicStore&& other) // moving raw data should be fine
    {
        if (this != &other) {
            if (m_isValid) {
                Handle::destroy();
            }

            if (other.get()) {
                std::ranges::move(other.m_buff, m_buff.begin());
                init();
                other.m_isValid = false;
                other.reset();
            }
        }
        return *this;
    }

    template<IsBaseOf<Base> Derived>
    StaticPolymorphicStore& operator=(const Derived& other) // converting assignment instead of copy assignment
    {
        if (this != &other) {
            if (m_isValid) {
                Handle::destroy();
            }

            if (other.get()) {
                Handle::clone(other);
                init();
            }
        }
        return *this;
    }

private:
    // ----------------------------------------
    // --- modification
    // ----------------------------------------
    void init() override { m_data = reinterpret_cast<Base*>(m_buff.data()); }
    void reset() override { m_data = nullptr; }

    // ----------------------------------------
    // --- data
    // ----------------------------------------
    StaticBuffer<Alloc.size, Alloc.align> m_buff = {};
    Base* m_data = nullptr;
    bool m_isValid = true;
};

namespace detail {
    template<typename Base, typename Derived>
    requires 
        (is_tuple_v<Derived> && is_base_of_all_v<Base, Derived>) ||
        ((!is_tuple_v<Derived>) && std::is_base_of_v<Base, Derived>)
    struct StaticPolymorphic
    {
        using DerivedTuple = std::conditional_t<
            is_tuple_v<Derived>,
            Derived,
            std::tuple<Derived>
        >;
        using type = StaticPolymorphicStore<Base, MaxAllocData<DerivedTuple>::data>;
    };
}

template<typename Base, typename Derived>
using StaticPolymorphic = typename detail::StaticPolymorphic<Base, Derived>::type;

//------------------------------------------------------
//                      Dynamic
//------------------------------------------------------

template<typename Base>
class DynamicPolymorphic : public IPolymorphic<Base>
{
    template<typename Base_Friend>
    friend class DynamicPolymorphic;

public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Handle = IPolymorphic<Base>;

    // ----------------------------------------
    // --- constructors/detructors
    // ----------------------------------------
    DynamicPolymorphic() : Handle(false) {}
    template <typename Derived, typename... Args>
    DynamicPolymorphic(Args&&... args)
        : Handle(false)
    {
        m_data = std::make_unique<Derived>(std::forward<Args>(args)...);
        Handle::m_type = &typeid(Derived);
    }

    DynamicPolymorphic(const DynamicPolymorphic&) = delete;
    DynamicPolymorphic(DynamicPolymorphic&& other)
        : Handle(false)
    {
        m_data = std::move(other.m_data);
        Handle::m_type = other.m_type;

        other.destroy();
    }

    ~DynamicPolymorphic()
    {
        Handle::destroy();
    }

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    constexpr size_t size() const override { return sizeof(m_data); }
    Base* get() override { return m_data.get(); }
    const Base* get() const override { return m_data.get(); }

    // ----------------------------------------
    // --- modification
    // ----------------------------------------
    void set(Base* data) override { m_data.reset(data); }

    DynamicPolymorphic& operator=(const DynamicPolymorphic&) = delete;
    DynamicPolymorphic& operator=(DynamicPolymorphic&& other)
    {
        if (this != &other) {
            m_data = std::move(other.m_data);
            Handle::m_type = other.m_type;

            other.destroy();
        }
        return *this;
    }

private:
    // ----------------------------------------
    // --- modification
    // ----------------------------------------
    void init() override {}
    void reset() override { m_data.reset(); }

    // ----------------------------------------
    // --- data
    // ----------------------------------------
    std::unique_ptr<Base> m_data = nullptr;
};