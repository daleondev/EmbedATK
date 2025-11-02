#pragma once

#include "Buffer.h"

//------------------------------------------------------
//                      ObjectStore
//------------------------------------------------------

template<typename T>
class IObjectStore
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using ValueType     = T;
    using Iterator      = std::span<ValueType>::iterator;
    using ConstIterator = std::span<const ValueType>::iterator;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    virtual ~IObjectStore() = default;

    // ----------------------------------------
    // --- iterators
    // ----------------------------------------
    virtual constexpr Iterator begin() = 0;
    virtual constexpr ConstIterator begin() const = 0;
    virtual constexpr Iterator end() = 0;
    virtual constexpr ConstIterator end() const = 0;

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    virtual constexpr size_t size() const = 0;

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    virtual constexpr ValueType* data() = 0;
    virtual constexpr const ValueType* data() const = 0;
    virtual constexpr ValueType& operator[](const size_t index) = 0;
    virtual constexpr const ValueType& operator[](const size_t index) const = 0;
    virtual constexpr ValueType& at(size_t index) = 0;
    virtual constexpr const ValueType& at(size_t index) const = 0;

    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    virtual void destroy(size_t index, size_t count) = 0;
    virtual void construct(size_t index, size_t count) = 0;
    virtual void fill(size_t index, size_t count, const ValueType& value) = 0;
    virtual void clone(const IObjectStore& other, size_t index, size_t count) = 0;
    virtual void transfer(IObjectStore&& other, size_t index, size_t count) = 0;
    virtual void uninitializedFill(size_t index, size_t count, const ValueType& value) = 0;
    virtual void uninitializedClone(const IObjectStore& other, size_t index, size_t count) = 0;
    virtual void uninitializedTransfer(IObjectStore&& other, size_t index, size_t count) = 0;
};

template<typename T, size_t N, bool ClearOnDestroy = true>
class StaticObjectStore : public IObjectStore<T>
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Handle        = IObjectStore<T>;
    using ValueType     = Handle::ValueType;
    using Iterator      = Handle::Iterator;
    using ConstIterator = Handle::ConstIterator;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    StaticObjectStore() : m_buff{}, m_data{reinterpret_cast<ValueType*>(m_buff.data()), N} {}
    StaticObjectStore(const StaticObjectStore&) = delete;
    StaticObjectStore(StaticObjectStore&&) = delete;
    virtual ~StaticObjectStore() = default;

    // ----------------------------------------
    // --- iterators
    // ----------------------------------------
    constexpr Iterator begin() override { return m_data.begin(); }
    constexpr ConstIterator begin() const override { return std::span<const ValueType>{m_data}.begin(); }
    constexpr Iterator end() override { return m_data.end(); }
    constexpr ConstIterator end() const override { return std::span<const ValueType>{m_data}.end(); }

    // ----------------------------------------
    // --- information
    // ----------------------------------------
     constexpr size_t size() const override { return m_data.size(); }

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    constexpr ValueType* data() override { return m_data.data(); }
    constexpr const ValueType* data() const override { return m_data.data(); }
    constexpr ValueType& operator[](const size_t index) override { return m_data[index]; }
    constexpr const ValueType& operator[](const size_t index) const override { return m_data[index]; }

    constexpr ValueType& at(size_t index) override 
    { 
        if (index >= N) throw std::out_of_range("index exceeds vector size");
        return m_data[index]; 
    }

    constexpr const ValueType& at(size_t index) const override 
    { 
        if (index >= N) throw std::out_of_range("index exceeds vector size");
        return m_data[index]; 
    }

    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    void destroy(size_t index, size_t count) override
    {
        std::ranges::destroy(m_data | std::views::drop(index) | std::views::take(count));
        if constexpr (ClearOnDestroy) m_buff.clearBlock(index, count);
    }

    void construct(size_t index, size_t count) override
    {
        if constexpr (std::is_default_constructible_v<ValueType>) {
            std::uninitialized_value_construct_n(data() + index, count);
        }
        else {
            throw std::logic_error("default construction not allowed");
        }
    }

    void fill(size_t index, size_t count, const ValueType& value) override
    {
        if constexpr (std::is_copy_assignable_v<ValueType>) {
            std::ranges::fill(m_data | std::views::drop(index) | std::views::take(count), value);
        }
        else {
            throw std::logic_error("copying not allowed");
        }
    }

    void clone(const Handle& other, size_t index, size_t count) override
    {
        if constexpr (std::is_copy_assignable_v<ValueType>) {
            std::ranges::copy(other | std::views::drop(index) | std::views::take(count), begin()+index);
        }
        else {
            throw std::logic_error("copying not allowed");
        }
    }

    void transfer(Handle&& other, size_t index, size_t count) override
    {
        if constexpr (std::is_move_assignable_v<ValueType>) {
            std::ranges::move(other | std::views::drop(index) | std::views::take(count), begin()+index);
        }
        else {
            throw std::logic_error("moving not allowed");
        }
    }

    void uninitializedFill(size_t index, size_t count, const ValueType& value) override
    {
        if constexpr (std::is_copy_constructible_v<ValueType>) {
            std::uninitialized_fill_n(data() + index, count, value);
        }
        else {
            throw std::logic_error("copying not allowed");
        }
    }

    void uninitializedClone(const IObjectStore<ValueType>& other, size_t index, size_t count) override
    {
        if constexpr (std::is_copy_constructible_v<ValueType>) {
            std::uninitialized_copy_n(other.data() + index, count, begin() + index);
        }
        else {
            throw std::logic_error("copying not allowed");
        }
    }

    void uninitializedTransfer(IObjectStore<ValueType>&& other, size_t index, size_t count) override
    {
        if constexpr (std::is_move_constructible_v<ValueType>) {
            std::uninitialized_move_n(other.data() + index, count, begin() + index);
        }
        else {
            throw std::logic_error("moving not allowed");
        }
    }

private:
    // ----------------------------------------
    // --- data
    // ----------------------------------------
    StaticBlockBuffer<N, allocData<ValueType>()> m_buff;
    std::span<T> m_data;
};