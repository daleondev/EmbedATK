#pragma once

#include "Memory.h"

// //------------------------------------------------------
// //                      Buffer
// //------------------------------------------------------

// class IBuffer
// {
// public:
//     // ----------------------------------------
//     // --- types
//     // ----------------------------------------
//     using ValueType     = std::byte;
//     using Iterator      = std::span<ValueType>::iterator;
//     using ConstIterator = std::span<const ValueType>::iterator;

//     // ----------------------------------------
//     // --- constructors/destructors
//     // ----------------------------------------
//     virtual ~IBuffer() = default;

//     // ----------------------------------------
//     // --- iterators
//     // ----------------------------------------
//     virtual constexpr Iterator begin() = 0;
//     virtual constexpr ConstIterator begin() const = 0;
//     virtual constexpr Iterator end() = 0;
//     virtual constexpr ConstIterator end() const = 0;

//     // ----------------------------------------
//     // --- information
//     // ----------------------------------------
//     virtual constexpr size_t size() const = 0;

//     // ----------------------------------------
//     // --- data access
//     // ----------------------------------------
//     virtual constexpr ValueType* data() = 0;
//     virtual constexpr const ValueType* data() const = 0;
//     virtual constexpr ValueType& operator[](const size_t index) = 0;
//     virtual constexpr const ValueType& operator[](const size_t index) const = 0;
//     virtual constexpr ValueType& at(size_t index) = 0;
//     virtual constexpr const ValueType& at(size_t index) const = 0;
//     virtual constexpr operator std::span<ValueType>() = 0;
//     virtual constexpr operator std::span<const ValueType>() const = 0;

//     // ----------------------------------------
//     // --- manipulation
//     // ----------------------------------------
//     virtual void clear(size_t index, size_t count) = 0;
//     virtual void fill(size_t index, size_t count, const ValueType& value) = 0;
// };

template<size_t N, size_t Align>
class StaticBuffer //: public IBuffer
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    // using Handle        = IBuffer;
    using ValueType     = std::byte;
    using Iterator      = std::span<ValueType>::iterator;
    using ConstIterator = std::span<const ValueType>::iterator;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    StaticBuffer() = default;
    StaticBuffer(const StaticBuffer&) = default;
    StaticBuffer(StaticBuffer&&) = default;
    ~StaticBuffer() = default;

    // ----------------------------------------
    // --- iterators
    // ----------------------------------------
    constexpr Iterator begin() { return std::span<ValueType>{m_data.data(), m_data.size()}.begin(); }
    constexpr ConstIterator begin() const { return std::span<const ValueType>{m_data.data(), m_data.size()}.begin(); }
    constexpr Iterator end() { return std::span<ValueType>{m_data.data(), m_data.size()}.end(); }
    constexpr ConstIterator end() const { return std::span<const ValueType>{m_data.data(), m_data.size()}.end(); }

    // ----------------------------------------
    // --- information
    // ----------------------------------------
    constexpr size_t size() const { return m_data.size(); }

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    constexpr ValueType* data() { return m_data.data(); }
    constexpr const ValueType* data() const { return m_data.data(); }
    constexpr ValueType& operator[](const size_t index) { return m_data[index]; }
    constexpr const ValueType& operator[](const size_t index) const { return m_data[index]; }
    constexpr ValueType& at(const size_t index) { return m_data.at(index); }
    constexpr const ValueType& at(const size_t index) const { return m_data.at(index); }
    constexpr operator std::span<ValueType>() { return m_data; }
    constexpr operator std::span<const ValueType>() const { return m_data; }
    
    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    StaticBuffer& operator=(const StaticBuffer&) = default;
    StaticBuffer& operator=(StaticBuffer&&) = default;

    void clear(size_t index, size_t count)
    {
        std::ranges::fill(m_data | std::views::drop(index) | std::views::take(count), ValueType{0x00});
    }

    void fill(size_t index, size_t count, const ValueType& value)
    {
        std::ranges::fill(m_data | std::views::drop(index) | std::views::take(count), value);
    }

protected:
    alignas(Align) std::array<ValueType, N> m_data;
};

template<size_t N, AllocData Block>
class StaticBlockBuffer : public StaticBuffer<Block.size*N, Block.align>
{
public:
    // ----------------------------------------
    // --- types
    // ----------------------------------------
    using Buffer        = StaticBuffer<Block.size*N, Block.align>;
    using ValueType     = Buffer::ValueType;

    // ----------------------------------------
    // --- constructors/destructors
    // ----------------------------------------
    StaticBlockBuffer() = default;
    StaticBlockBuffer(const StaticBlockBuffer&) = default;
    StaticBlockBuffer(StaticBlockBuffer&&) = default;
    ~StaticBlockBuffer() = default;

    // ----------------------------------------
    // --- data access
    // ----------------------------------------
    constexpr ValueType& atBlock(const size_t index) { return Buffer::at(Block.size*index); }
    constexpr const ValueType& atBlock(const size_t index) const { return Buffer::at(Block.size*index); }
    
    // ----------------------------------------
    // --- manipulation
    // ----------------------------------------
    StaticBlockBuffer& operator=(const StaticBlockBuffer&) = default;
    StaticBlockBuffer& operator=(StaticBlockBuffer&&) = default;

    void clearBlock(size_t index, size_t count)
    {
        Buffer::clear(Block.size*index, Block.size*count);
    }

    void fillBlock(size_t index, size_t count, const ValueType& value)
    {
        Buffer::fill(Block.size*index, Block.size*count, value);
    }
};