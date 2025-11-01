#include "EmbedATK/EmbedATK.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}

//------------------------------------------------------
//                      AllocData
//------------------------------------------------------
TEST(AllocDataTest, SimpleType)
{
	constexpr auto data = allocData<int>();
	EXPECT_EQ(data.size, sizeof(int));
	EXPECT_EQ(data.align, std::max(alignof(int), alignof(uintptr_t)));
}

struct alignas(16) MyStruct
{
	int a;
	char b;
};

TEST(AllocDataTest, CustomStruct)
{
	constexpr auto data = allocData<MyStruct>();
	EXPECT_EQ(data.size, std::max(sizeof(MyStruct), sizeof(uintptr_t)));
	EXPECT_EQ(data.align, std::max(alignof(MyStruct), alignof(uintptr_t)));
	EXPECT_EQ(data.align, 16);
}

TEST(AllocDataTest, Max)
{
	using Types = std::tuple<int, std::array<std::byte, 34>, MyStruct>;
	constexpr auto data = MaxAllocData<Types>::data;

	std::array<size_t, 3> sizes = {
		sizeof(int), sizeof(std::array<std::byte, 34>), sizeof(MyStruct)
	};

	std::array<size_t, 3> aligns = {
		alignof(int), alignof(std::array<std::byte, 34>), alignof(MyStruct)
	};

	EXPECT_EQ(data.size, std::ranges::max(sizes));
	EXPECT_EQ(data.align, std::ranges::max(aligns));
}

//------------------------------------------------------
//                      Buffer
//------------------------------------------------------
TEST(StaticBufferTest, Initialization)
{
	StaticBuffer<128, 16> buffer;
	EXPECT_EQ(buffer.size(), 128);
	ASSERT_NE(buffer.data(), nullptr);
}

TEST(StaticBufferTest, DataAccess)
{
	StaticBuffer<16, 4> buffer;
	buffer[0] = std::byte{0xAB};
	EXPECT_EQ(buffer[0], std::byte{0xAB});
	EXPECT_EQ(buffer.at(0), std::byte{0xAB});

	buffer.at(1) = std::byte{0xCD};
	EXPECT_EQ(buffer[1], std::byte{0xCD});

	EXPECT_THROW(buffer.at(16), std::out_of_range);
}

TEST(StaticBufferTest, Manipulation)
{
	StaticBuffer<32, 8> buffer;
	const auto val1 = std::byte{0xFF};
	buffer.fill(0, buffer.size(), val1);
	for (size_t i = 0; i < buffer.size(); ++i)
	{
		EXPECT_EQ(buffer[i], val1);
	}

	buffer.clear(8, 16);
	for (size_t i = 0; i < 8; ++i)
	{
		EXPECT_EQ(buffer[i], val1);
	}
	for (size_t i = 8; i < 24; ++i)
	{
		EXPECT_EQ(buffer[i], std::byte{0x00});
	}
	for (size_t i = 24; i < 32; ++i)
	{
		EXPECT_EQ(buffer[i], val1);
	}
}

TEST(StaticBufferTest, Iterators)
{
	StaticBuffer<10, 1> buffer;
	EXPECT_EQ(std::distance(buffer.begin(), buffer.end()), 10);

	const auto& cbuffer = buffer;
	EXPECT_EQ(std::distance(cbuffer.begin(), cbuffer.end()), 10);
}

TEST(StaticBlockBufferTest, BlockManipulation)
{
	constexpr auto blockData = allocData<uint32_t>(); // size 4, align 8
	StaticBlockBuffer<10, blockData> buffer;		 // 10 blocks of 4 bytes = 40 bytes total

	EXPECT_EQ(buffer.size(), 40);

	const auto val1 = std::byte{0xAA};
	buffer.fillBlock(2, 3, val1); // fill 3 blocks starting from index 2

	for (size_t i = 0; i < 2 * blockData.size; ++i)
	{
		EXPECT_EQ(buffer[i], std::byte{0x00}); // unchanged
	}

	for (size_t i = 2 * blockData.size; i < 5 * blockData.size; ++i)
	{
		EXPECT_EQ(buffer[i], val1); // filled
	}

	for (size_t i = 5 * blockData.size; i < 10 * blockData.size; ++i)
	{
		EXPECT_EQ(buffer[i], std::byte{0x00}); // unchanged
	}

	buffer.clearBlock(3, 2); // clear 2 blocks starting from index 3

	for (size_t i = 3 * blockData.size; i < 5 * blockData.size; ++i)
	{
		EXPECT_EQ(buffer[i], std::byte{0x00}); // cleared
	}
}

//------------------------------------------------------
//                      ObjectStore
//------------------------------------------------------
struct TestObject
{
	static int construction_count;
	static int destruction_count;
	static int copy_construction_count;
	static int move_construction_count;

	int value;
	bool moved_from;

	TestObject(int v = 0) : value(v), moved_from(false) { construction_count++; }
	~TestObject() { destruction_count++; }

	TestObject(const TestObject& other) : value(other.value), moved_from(false)
	{
		construction_count++;
		copy_construction_count++;
	}
	TestObject& operator=(const TestObject& other)
	{
		value = other.value;
		moved_from = false;
		return *this;
	}

	TestObject(TestObject&& other) noexcept : value(other.value), moved_from(false)
	{
		other.moved_from = true;
		construction_count++;
		move_construction_count++;
	}
	TestObject& operator=(TestObject&& other) noexcept
	{
		value = other.value;
		moved_from = false;
		other.moved_from = true;
		return *this;
	}

	bool operator==(const TestObject& other) const { return value == other.value; }

	static void reset_counts()
	{
		construction_count = 0;
		destruction_count = 0;
		copy_construction_count = 0;
		move_construction_count = 0;
	}
};

int TestObject::construction_count = 0;
int TestObject::destruction_count = 0;
int TestObject::copy_construction_count = 0;
int TestObject::move_construction_count = 0;

class StaticObjectStoreTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		TestObject::reset_counts();
	}
};

TEST_F(StaticObjectStoreTest, Initialization)
{
	StaticObjectStore<TestObject, 10> store;
	EXPECT_EQ(store.size(), 10);
	EXPECT_EQ(TestObject::construction_count, 0);
	EXPECT_EQ(TestObject::destruction_count, 0);
}

TEST_F(StaticObjectStoreTest, ConstructDestroy)
{
	StaticObjectStore<TestObject, 5> store;
	store.construct(0, 5);
	EXPECT_EQ(TestObject::construction_count, 5);
	for (size_t i = 0; i < 5; ++i)
	{
		EXPECT_EQ(store[i].value, 0);
	}

	store.destroy(0, 5);
	EXPECT_EQ(TestObject::destruction_count, 5);
}

TEST_F(StaticObjectStoreTest, Fill)
{
	StaticObjectStore<TestObject, 5> store;
	store.construct(0, 5);
	EXPECT_EQ(TestObject::construction_count, 5);

	store.fill(1, 3, TestObject{42});
	EXPECT_EQ(TestObject::construction_count, 6); // one temporary for fill value

	EXPECT_EQ(store[0].value, 0);
	EXPECT_EQ(store[1].value, 42);
	EXPECT_EQ(store[2].value, 42);
	EXPECT_EQ(store[3].value, 42);
	EXPECT_EQ(store[4].value, 0);

	store.destroy(0, 5);
	EXPECT_EQ(TestObject::destruction_count, 6);
}

TEST_F(StaticObjectStoreTest, UninitializedFill)
{
	StaticObjectStore<TestObject, 5> store;
	store.uninitializedFill(0, 3, TestObject{123});
	EXPECT_EQ(TestObject::construction_count, 4); // 1 temp + 3 copies
	EXPECT_EQ(TestObject::copy_construction_count, 3);

	for (size_t i = 0; i < 3; ++i)
	{
		EXPECT_EQ(store[i].value, 123);
	}

	store.destroy(0, 3);
	EXPECT_EQ(TestObject::destruction_count, 4);
}

TEST_F(StaticObjectStoreTest, Access)
{
	StaticObjectStore<int, 5> store;
	store.construct(0, 5);
	for (int i = 0; i < 5; ++i)
		store[i] = i * i;

	EXPECT_EQ(store[2], 4);
	EXPECT_EQ(store.at(3), 9);
	EXPECT_THROW(store.at(5), std::out_of_range);

	store.destroy(0, 5);
}

TEST_F(StaticObjectStoreTest, UninitializedCloneCorrectBehavior)
{
	StaticObjectStore<TestObject, 5> store1;
	store1.construct(0, 5);
	for (size_t i = 0; i < 5; ++i)
		store1[i].value = i;

	StaticObjectStore<TestObject, 5> store2;

	TestObject::reset_counts();

	store2.uninitializedClone(store1, 1, 3);

	EXPECT_EQ(TestObject::copy_construction_count, 3);
	EXPECT_EQ(store2[1].value, 1);
	EXPECT_EQ(store2[2].value, 2);
	EXPECT_EQ(store2[3].value, 3);

	store1.destroy(0, 5);
	store2.destroy(1, 3);
}

TEST_F(StaticObjectStoreTest, UninitializedTransferCorrectBehavior)
{
	StaticObjectStore<TestObject, 5> store1;
	store1.construct(0, 5);
	for (size_t i = 0; i < 5; ++i)
		store1[i].value = i;

	StaticObjectStore<TestObject, 5> store2;

	TestObject::reset_counts();

	store2.uninitializedTransfer(std::move(store1), 1, 3);

	EXPECT_EQ(TestObject::move_construction_count, 3);
	EXPECT_EQ(store2[1].value, 1);
	EXPECT_EQ(store2[2].value, 2);
	EXPECT_EQ(store2[3].value, 3);

	// Check if original objects were marked as moved from
	EXPECT_TRUE(store1[1].moved_from);
	EXPECT_TRUE(store1[2].moved_from);
	EXPECT_TRUE(store1[3].moved_from);

	store1.destroy(0, 5);
	store2.destroy(1, 3);
}

//------------------------------------------------------
//                    PMR Unique Ptr
//------------------------------------------------------
struct StaticTestObject
{
	static bool destroyed;
	int x, y;

	StaticTestObject(int val1, int val2) : x(val1), y(val2) { destroyed = false; }
	~StaticTestObject() { destroyed = true; }
};
bool StaticTestObject::destroyed = false;

TEST(StaticUniquePtrTest, MakeAndDestroy)
{
	std::array<std::byte, 256> buffer;
	std::pmr::monotonic_buffer_resource resource{buffer.data(), buffer.size()};

	StaticTestObject::destroyed = false;

	{
		auto ptr = make_static_unique<StaticTestObject>(&resource, 10, 20);
		EXPECT_EQ(ptr->x, 10);
		EXPECT_EQ(ptr->y, 20);
		EXPECT_FALSE(StaticTestObject::destroyed);
	}

	EXPECT_TRUE(StaticTestObject::destroyed);
}

TEST(StaticUniquePtrTest, Deleter)
{
	std::array<std::byte, 256> buffer;
	std::pmr::monotonic_buffer_resource resource{buffer.data(), buffer.size()};
	void* mem = resource.allocate(sizeof(StaticTestObject), alignof(StaticTestObject));
	StaticTestObject* raw_ptr = new (mem) StaticTestObject(1, 2);

	StaticTestObject::destroyed = false;

	{
		StaticUniquePtr<StaticTestObject> ptr(raw_ptr, StaticDeleter<StaticTestObject>{&resource});
		EXPECT_FALSE(StaticTestObject::destroyed);
	}

	EXPECT_TRUE(StaticTestObject::destroyed);
}

//------------------------------------------------------
//                      Pools
//------------------------------------------------------

// --- StaticBlockPool ---
TEST(StaticBlockPoolTest, BasicAllocationDeallocation)
{
	constexpr auto blockData = allocData<double>();
	StaticBlockPool<10, blockData> pool;

	void* p = pool.allocate(sizeof(double), alignof(double));
	ASSERT_NE(p, nullptr);

	// Check if pointer is within the buffer
	const void* pool_data_start = pool.data();
	const void* pool_data_end = static_cast<const std::byte*>(pool.data()) + (10 * blockData.size);
	EXPECT_GE(p, pool_data_start);
	EXPECT_LT(p, pool_data_end);

	pool.deallocate(p, sizeof(double), alignof(double));
}

TEST(StaticBlockPoolTest, AllocateAllBlocks)
{
	constexpr auto blockData = allocData<double>();
	StaticBlockPool<5, blockData> pool;
	std::vector<void*> allocations;

	for (int i = 0; i < 5; ++i)
	{
		void* p = pool.allocate(sizeof(double), alignof(double));
		ASSERT_NE(p, nullptr);
		allocations.push_back(p);
	}

	EXPECT_THROW(pool.allocate(sizeof(double), alignof(double)), std::bad_alloc);

	for (void* p : allocations)
	{
		pool.deallocate(p, sizeof(double), alignof(double));
	}
}

TEST(StaticBlockPoolTest, DeallocateAndReallocate)
{
	constexpr auto blockData = allocData<double>();
	StaticBlockPool<2, blockData> pool;

	void* p1 = pool.allocate(sizeof(double), alignof(double));
	void* p2 = pool.allocate(sizeof(double), alignof(double));
	EXPECT_THROW(pool.allocate(sizeof(double), alignof(double)), std::bad_alloc);

	pool.deallocate(p1, sizeof(double), alignof(double));

	void* p3 = pool.allocate(sizeof(double), alignof(double));
	EXPECT_NE(p3, nullptr);

	pool.deallocate(p2, sizeof(double), alignof(double));
	pool.deallocate(p3, sizeof(double), alignof(double));
}

TEST(StaticBlockPoolTest, InvalidOperations)
{
	constexpr auto blockData = allocData<long>();
	StaticBlockPool<10, blockData> pool;

	void* p = pool.allocate(sizeof(long), alignof(long));

	// Invalid size/alignment on allocate
	EXPECT_THROW(pool.allocate(blockData.size + 1, blockData.align), std::bad_alloc);

	// Invalid deallocation
	EXPECT_THROW(pool.deallocate(nullptr, blockData.size, blockData.align), std::invalid_argument);
	int x;
	EXPECT_THROW(pool.deallocate(&x, blockData.size, blockData.align), std::invalid_argument); // outside pool


	pool.deallocate(p, sizeof(long), alignof(long));
}

// --- StaticEntiredPool ---
TEST(StaticEntiredPoolTest, BasicAllocationDeallocation)
{
	StaticEntiredPool<int, 10> pool; // 10 * sizeof(int)

	void* p = pool.allocate(10 * sizeof(int), alignof(int));
	ASSERT_NE(p, nullptr);
	EXPECT_EQ(p, pool.data());

	pool.deallocate(p, 10 * sizeof(int), alignof(int));
}

TEST(StaticEntiredPoolTest, OverAllocation)
{
	StaticEntiredPool<int, 10> pool;

	EXPECT_THROW(pool.allocate(100, 1), std::bad_alloc);
	auto alloc = allocData<int>();
	if (alloc.align < 16)
	{
		EXPECT_THROW(pool.allocate(80, 16), std::bad_alloc); // bad alignment
	}
}

TEST(StaticEntiredPoolTest, SingleAllocationBehavior)
{
	StaticEntiredPool<int, 10> pool;
	const size_t total_size = 10 * sizeof(uintptr_t);

	void* p1 = pool.allocate(total_size, alignof(uintptr_t));
	ASSERT_NE(p1, nullptr);

	pool.deallocate(p1, total_size, alignof(uintptr_t));
}

TEST(StaticEntiredPoolTest, InvalidDeallocation)
{
	StaticEntiredPool<int, 5> pool;
	const size_t total_size = 5 * sizeof(double);

	void* p = pool.allocate(total_size, alignof(double));

	EXPECT_THROW(pool.deallocate(nullptr, total_size, alignof(double)), std::invalid_argument);
	int x;
	EXPECT_THROW(pool.deallocate(&x, total_size, alignof(double)), std::invalid_argument);
	double* p_offset = static_cast<double*>(p) + 1;
	EXPECT_THROW(pool.deallocate(p_offset, total_size, alignof(double)), std::invalid_argument);

	pool.deallocate(p, total_size, alignof(double));
}

// --- StaticMonotonicPool ---
TEST(StaticMonotonicPoolTest, SequentialAllocation)
{
	StaticMonotonicPool<int, 10> pool; // 10 * sizeof(int) bytes

	void* p1 = pool.allocate(sizeof(int), alignof(int));
	void* p2 = pool.allocate(sizeof(int), alignof(int));
	ASSERT_NE(p1, nullptr);
	ASSERT_NE(p2, nullptr);
	EXPECT_NE(p1, p2);

	uintptr_t addr1 = reinterpret_cast<uintptr_t>(p1);
	uintptr_t addr2 = reinterpret_cast<uintptr_t>(p2);
	EXPECT_GE(addr2, addr1 + sizeof(int));
}

TEST(StaticMonotonicPoolTest, OverAllocation)
{
	StaticMonotonicPool<char, 50> pool;
	pool.allocate(50, 1);
	EXPECT_THROW(pool.allocate(1, 1), std::bad_alloc);

	StaticMonotonicPool<char, 50> pool2;
	EXPECT_THROW(pool2.allocate(51, 1), std::bad_alloc);
}

TEST(StaticMonotonicPoolTest, DeallocationIsNoOp)
{
	StaticMonotonicPool<int, 2> pool;
	void* p1 = pool.allocate(sizeof(int), alignof(int));
	void* p2 = pool.allocate(sizeof(int), alignof(int));

	pool.deallocate(p2, sizeof(int), alignof(int)); // Should do nothing

	// Cannot allocate more
	EXPECT_THROW(pool.allocate(sizeof(int), alignof(int)), std::bad_alloc);

	pool.deallocate(p1, sizeof(int), alignof(int));
}

#include "polymorphic_tests.h"
#include "containers_tests.h"