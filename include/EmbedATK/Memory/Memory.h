#pragma once

//------------------------------------------------------
//                      Alloc Data
//------------------------------------------------------

struct AllocData
{
    size_t size     = 0;
    size_t align    = alignof(uintptr_t);
};

template<typename T>
static consteval AllocData allocData() 
{ 
    return AllocData
    { 
        .size   = sizeof(T),
        .align  = std::max(alignof(T), alignof(uintptr_t))
    }; 
}

constexpr bool operator==(const AllocData& a, const AllocData& b)
{
    return a.size == b.size && a.align == b.align;
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