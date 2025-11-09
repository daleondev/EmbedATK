#pragma once

#include "Buffer.h"

//------------------------------------------------------
//                    Any
//------------------------------------------------------

class SboAny
{
public:
    SboAny() noexcept = default;

    template <typename T, typename... Args>
    SboAny(std::in_place_type_t<T> tag, Args&&... args)
    {
        m_any.emplace<T>(std::forward<Args>(args)...);

        const uintptr_t any_start = reinterpret_cast<uintptr_t>(&m_any);
        const uintptr_t any_end = any_start + sizeof(SboAny);

        const uintptr_t value_ptr = reinterpret_cast<uintptr_t>(std::any_cast<T>(&m_any));
        const uintptr_t value_end = value_ptr + sizeof(T);
        
        bool is_in_buffer = (value_ptr >= any_start) && (value_end <= any_end);
        if (!is_in_buffer) {
            throw std::bad_alloc();
        }
    }

    SboAny(SboAny&& other) noexcept = default;
    SboAny& operator=(SboAny&& other) noexcept = default;

    SboAny(const SboAny&) = delete;
    SboAny& operator=(const SboAny&) = delete;

    bool has_value() const noexcept 
    {
        return m_any.has_value();
    }
    void reset() noexcept 
    {
        m_any.reset();
    }

    const std::type_info& type() const noexcept
    {
        return m_any.type();
    }

    std::any& any() 
    {
        return m_any;
    }

    const std::any& any() const 
    {
        return m_any;
    }

    template <typename T>
    const T& as() const
    {
        return std::any_cast<const T&>(m_any);
    }

    template <typename T>
    T& as()
    {
        return std::any_cast<T&>(m_any);
    }

private:
    std::any m_any;
};

template <size_t N>
class StaticAny 
{
public:
    constexpr StaticAny() noexcept = default;

    StaticAny(const StaticAny& other) 
    {
        if (other.hasValue()) {
            other.m_ctrl->copy(&other, this);
        }
    }

    StaticAny(StaticAny&& other) noexcept 
    {
        if (other.hasValue()) {
            other.m_ctrl->move(&other, this);
        }
    }

    template <typename ValueType, typename DecayedT = std::decay_t<ValueType>>
    requires (!std::is_same_v<DecayedT, StaticAny>) &&
            //  (!std::in_place_type_t<DecayedT>) &&
             (std::is_copy_constructible_v<DecayedT>) &&
             (std::is_constructible_v<DecayedT, ValueType>)
    StaticAny(ValueType&& value) 
    {
        emplace<DecayedT>(std::forward<ValueType>(value));
    }

    template <typename T, typename... Args>
    requires(std::is_constructible_v<T, Args...>) &&
            (std::is_copy_constructible_v<T>)
    explicit StaticAny(std::in_place_type_t<T>, Args&&... args) 
    {
        emplace<T>(std::forward<Args>(args)...);
    }

    template <typename T, typename U, typename... Args>
    requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...>) &&
            (std::is_copy_constructible_v<T>)
    explicit StaticAny(std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args) 
    {
        emplace<T>(il, std::forward<Args>(args)...);
    }

    ~StaticAny() {
        reset();
    }

    StaticAny& operator=(const StaticAny& other) 
    {
        if (this != &other) {
            reset();
            if (other.hasValue()) {
                other.m_ctrl->copy(&other, this);
            }
        }
        return *this;
    }

    StaticAny& operator=(StaticAny&& other) noexcept 
    {
        if (this != &other) {
            reset();
            if (other.hasValue()) {
                other.m_ctrl->move(&other, this);
            }
        }
        return *this;
    }

    template <typename ValueType, typename DecayedT = std::decay_t<ValueType>>
    requires(!std::is_same_v<DecayedT, StaticAny>) &&
            // (!std::is_in_place_type_v<DecayedT>) &&
            (std::is_copy_constructible_v<DecayedT>) &&
            (std::is_constructible_v<DecayedT, ValueType>)
    StaticAny& operator=(ValueType&& value) 
    {
        emplace<DecayedT>(std::forward<ValueType>(value));
        return *this;
    }

    template <typename T, typename... Args>
    requires(std::is_constructible_v<T, Args...>) &&
            (std::is_copy_constructible_v<T>)
    T& emplace(Args&&... args) 
    {
        validateStorage<T>();
        reset();
        T* newObj = std::construct_at(asUncheckedPtr<T>(), std::forward<Args>(args)...);
        m_ctrl = &ctrlFor<T>;
        return *newObj;
    }

    template <typename T, typename U, typename... Args>
    requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...>) &&
            (std::is_copy_constructible_v<T>)
    T& emplace(std::initializer_list<U> il, Args&&... args) 
    {
        validateStorage<T>();
        reset();
        T* newObj = std::construct_at(asUncheckedPtr<T>(), il, std::forward<Args>(args)...);
        m_ctrl = &ctrlFor<T>;
        return *newObj;
    }

    void reset() noexcept 
    {
        if (hasValue()) {
            m_ctrl->destroy(this);
            m_ctrl = &emptyCtrl;
        }
    }

    void swap(StaticAny& other) noexcept 
    {
        StaticAny temp = std::move(*this);
        *this = std::move(other);
        other = std::move(temp);
    }

    bool hasValue() const noexcept 
    {
        return m_ctrl != &emptyCtrl;
    }

    operator bool() const noexcept 
    {
        return hasValue();
    }

    const std::type_info& type() const noexcept 
    {
        return m_ctrl->type_info();
    }

    template <typename T>
    T* asUncheckedPtr() 
    {
        return reinterpret_cast<T*>(m_buff.data());
    }

    template <typename T>
    const T* asUncheckedPtr() const 
    {
        return reinterpret_cast<const T*>(m_buff.data());
    }

    template <typename T>
    const T& asUnchecked() const
    {
        return *asUncheckedPtr<T>();
    }

    template <typename T>
    T& asUnchecked()
    {
        return *asUncheckedPtr<T>();
    }

    template <typename T>
    const T* asPtr() const
    {
        if (type() != typeid(T)) {
            return nullptr;
        }
        return asPtr<T>();
    }

    template <typename T>
    T* asPtr()
    {
        if (type() != typeid(T)) {
            return nullptr;
        }
        return asPtr<T>();
    }

    template <typename T>
    std::optional<std::reference_wrapper<const T>> as() const
    {
        auto* ptr = asPtr<T>();
        return ptr == nullptr ? 
            std::nullopt : 
            std::ref(*ptr);
    }

    template <typename T>
    std::optional<std::reference_wrapper<T>> as()
    {
        auto* ptr = asPtr<T>();
        return ptr == nullptr ? 
            std::nullopt : 
            std::ref(*ptr);
    }

private:
    struct ControlBlock {
        void (*destroy)(StaticAny* self) = nullptr;
        void (*copy)(const StaticAny* from, StaticAny* to) = nullptr;
        void (*move)(StaticAny* from, StaticAny* to) = nullptr;
        const std::type_info& (*type_info)() noexcept = nullptr;
    };

    template <typename T>
    inline static constexpr ControlBlock ctrlFor = {
        .destroy = [](StaticAny* self) {
            if (self->hasValue()) {
                std::destroy_at(self->asUncheckedPtr<T>());
            }
        },
        .copy = [](const StaticAny* from, StaticAny* to) {
            std::construct_at(to->asUncheckedPtr<T>(), from->asUnchecked<T>());
            to->m_ctrl = from->m_ctrl;
        },
        .move = [](StaticAny* from, StaticAny* to) {
            std::construct_at(to->asUncheckedPtr<T>(), std::move(from->asUnchecked<T>()));
            to->m_ctrl = from->m_ctrl;
            from->reset();
        },
        .type_info = []() noexcept -> const std::type_info& {
            return typeid(T);
        }
    };

    inline static constexpr ControlBlock emptyCtrl = {
        .destroy = nullptr,
        .copy = nullptr,
        .move = nullptr,
        .type_info = []() noexcept -> const std::type_info& {
            return typeid(void);
        }
    };

    template <typename T>
    static constexpr void validateStorage() 
    {
        static_assert(sizeof(T) <= N,
            "Type is too large for this StaticAny's buffer size.");
        static_assert(alignof(T) <= alignof(std::max_align_t),
            "Type has an alignment requirement greater than std::max_align_t.");
    }

    const ControlBlock* m_ctrl = &emptyCtrl;
    StaticBuffer<N, alignof(std::max_align_t)> m_buff;
};