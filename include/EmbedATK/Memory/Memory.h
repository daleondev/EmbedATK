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
