// #pragma once

// #include "KitCAT/Utils/Containers.h"
// #include <gtest/gtest.h>
// #include <numeric>

// // Test fixture for container tests
// class ContainersTest : public ::testing::Test
// {
// protected:
// 	struct TestCounter
// 	{
// 		static int constructions;
// 		static int destructions;
// 		static int copies;
// 		static int moves;

// 		int value;

// 		TestCounter(int v = 0) : value(v) { constructions++; }
// 		~TestCounter() { destructions++; value = -1; }
// 		TestCounter(const TestCounter& o) : value(o.value)
// 		{
// 			constructions++;
// 			copies++;
// 		}
// 		TestCounter(TestCounter&& o) noexcept : value(o.value)
// 		{
// 			constructions++;
// 			moves++;
// 			o.value = -1;
// 		}
// 		TestCounter& operator=(const TestCounter& o)
// 		{
// 			value = o.value;
// 			copies++;
// 			return *this;
// 		}
// 		TestCounter& operator=(TestCounter&& o) noexcept
// 		{
// 			value = o.value;
// 			moves++;
// 			o.value = -1;
// 			return *this;
// 		}

// 		static void reset()
// 		{
// 			constructions = 0;
// 			destructions = 0;
// 			copies = 0;
// 			moves = 0;
// 		}
// 	};

// 	void SetUp() override { TestCounter::reset(); }
// };

// int ContainersTest::TestCounter::constructions = 0;
// int ContainersTest::TestCounter::destructions = 0;
// int ContainersTest::TestCounter::copies = 0;
// int ContainersTest::TestCounter::moves = 0;

// //------------------------------------------------------
// //                      AnyIterator
// //------------------------------------------------------
// TEST_F(ContainersTest, AnyIterator_Vector)
// {
// 	std::vector<int> vec = {10, 20, 30};
// 	AnyIterator<int> it(vec.begin());
// 	AnyIterator<int> end(vec.end());

// 	EXPECT_EQ(*it, 10);
// 	++it;
// 	EXPECT_EQ(*it, 20);
// 	EXPECT_NE(it, end);
// 	it++;
// 	EXPECT_EQ(*it, 30);
// 	++it;
// 	EXPECT_EQ(it, end);
// }

// TEST_F(ContainersTest, AnyIterator_Copy)
// {
// 	std::vector<int> vec = {10, 20};
// 	AnyIterator<int> it1(vec.begin());
// 	AnyIterator<int> it2 = it1; // Copy constructor

// 	EXPECT_EQ(it1, it2);
// 	EXPECT_EQ(*it1, 10);
// 	EXPECT_EQ(*it2, 10);

// 	++it1;
// 	EXPECT_NE(it1, it2);
// 	EXPECT_EQ(*it1, 20);
// 	EXPECT_EQ(*it2, 10);

// 	it2 = it1; // Copy assignment
// 	EXPECT_EQ(it1, it2);
// 	EXPECT_EQ(*it2, 20);
// }

// //------------------------------------------------------
// //                      StaticVector
// //------------------------------------------------------
// TEST_F(ContainersTest, StaticVector_Lifecycle)
// {
// 	{
// 		StaticVector<TestCounter, 5> vec;
// 		EXPECT_TRUE(vec.empty());
// 		EXPECT_EQ(vec.size(), 0);
// 		EXPECT_EQ(vec.capacity(), 5);

// 		vec.emplace_back(1);
// 		vec.push_back(TestCounter(2));
//         vec.push_back(std::move(TestCounter(3)));
//         TestCounter tmp(4);
//         vec.push_back(tmp);

// 		EXPECT_EQ(vec.size(), 4);
// 		EXPECT_EQ(TestCounter::constructions, 7); // 1 emplace_back, 2 push_back copy, 2 push_back move, 1 construct tmp, 1 copy tmp
// 		EXPECT_EQ(vec[0].value, 1);
// 		EXPECT_EQ(vec[1].value, 2);
// 		EXPECT_EQ(vec[2].value, 3);

// 		vec.erase(vec.begin() + 1);
// 		EXPECT_EQ(vec.size(), 3);
// 		EXPECT_EQ(vec[1].value, 3);
// 		EXPECT_EQ(TestCounter::destructions, 3); // 1 push_back copy, 1 push_back move, 1 erase

// 		vec.clear();
// 		EXPECT_TRUE(vec.empty());
// 		EXPECT_EQ(TestCounter::destructions, 6); // 3 + 3 from clear
// 	}
// 	EXPECT_EQ(TestCounter::constructions, TestCounter::destructions);
// }

// TEST_F(ContainersTest, StaticVector_Throws)
// {
// 	StaticVector<int, 2> vec;
// 	vec.push_back(1);
// 	vec.push_back(2);
// 	EXPECT_TRUE(vec.full());
// 	EXPECT_THROW(vec.push_back(3), std::length_error);
// 	EXPECT_THROW(vec.at(2), std::out_of_range);
// }

// TEST_F(ContainersTest, StaticVector_CopyOperations)
// {
// 	StaticVector<TestCounter, 5> original;
// 	original.push_back(1);
// 	original.push_back(2);
// 	original.push_back(3);
// 	TestCounter::reset();

// 	// Test Copy Constructor
// 	{
// 		StaticVector<TestCounter, 5> copy = original;
// 		EXPECT_EQ(TestCounter::constructions, 3);
// 		EXPECT_EQ(TestCounter::copies, 3);
// 		EXPECT_EQ(TestCounter::moves, 0);
// 		EXPECT_EQ(copy.size(), original.size());
// 		for (size_t i = 0; i < copy.size(); ++i)
// 		{
// 			EXPECT_EQ(copy[i].value, original[i].value);
// 		}
// 	} // copy is destroyed
// 	EXPECT_EQ(TestCounter::destructions, 3);
// 	TestCounter::reset();

// 	// Test Copy Assignment
// 	{
// 		StaticVector<TestCounter, 5> target;
// 		target.push_back(100);
// 		target.push_back(200);
// 		TestCounter::reset();

// 		target = original;
// 		EXPECT_EQ(TestCounter::copies, 3);
// 		EXPECT_EQ(TestCounter::moves, 0);
// 		// constructions/destructions depend on implementation (overwrite vs uninit_copy)
// 		EXPECT_EQ(target.size(), original.size());
// 		for (size_t i = 0; i < target.size(); ++i)
// 		{
// 			EXPECT_EQ(target[i].value, original[i].value);
// 		}
// 	}
// }

// TEST_F(ContainersTest, StaticVector_MoveOperations)
// {
// 	// Test Move Constructor
// 	{
// 		StaticVector<TestCounter, 5> original;
// 		original.emplace_back(1);
// 		original.emplace_back(2);
// 		original.emplace_back(3);
// 		TestCounter::reset();

// 		StaticVector<TestCounter, 5> moved = std::move(original);
// 		EXPECT_EQ(TestCounter::constructions, 3);
// 		EXPECT_EQ(TestCounter::copies, 0);
// 		EXPECT_EQ(TestCounter::moves, 3);
// 		EXPECT_EQ(moved.size(), 3);
// 		EXPECT_EQ(original.size(), 0); // Original should be cleared
// 		EXPECT_EQ(moved[0].value, 1);
// 		EXPECT_EQ(moved[1].value, 2);
// 		EXPECT_EQ(moved[2].value, 3);
// 	}
// 	EXPECT_EQ(2*TestCounter::constructions, TestCounter::destructions); // destroyed twice -> on move and on destruction
// 	TestCounter::reset();

// 	// Test Move Assignment
// 	{
// 		StaticVector<TestCounter, 5> original;
// 		original.emplace_back(1);
// 		original.emplace_back(2);
// 		original.emplace_back(3);

// 		StaticVector<TestCounter, 5> target;
// 		target.emplace_back(100);
// 		target.emplace_back(200);
// 		TestCounter::reset();

// 		target = std::move(original);
// 		EXPECT_EQ(TestCounter::copies, 0);
// 		EXPECT_EQ(TestCounter::moves, 3);
// 		EXPECT_EQ(target.size(), 3);
// 		EXPECT_EQ(original.size(), 0);
// 		EXPECT_EQ(target[0].value, 1);
// 		EXPECT_EQ(target[1].value, 2);
// 		EXPECT_EQ(target[2].value, 3);
// 	}
// }

// //------------------------------------------------------
// //                      StaticQueue
// //------------------------------------------------------
// TEST_F(ContainersTest, StaticQueue_Lifecycle)
// {
// 	StaticQueue<int, 3> q;
// 	EXPECT_TRUE(q.empty());
// 	EXPECT_EQ(q.size(), 0);
// 	EXPECT_EQ(q.capacity(), 3);

// 	EXPECT_TRUE(q.push(10));
// 	EXPECT_TRUE(q.push(20));
// 	EXPECT_TRUE(q.push(30));

// 	EXPECT_EQ(q.size(), 3);
// 	EXPECT_TRUE(q.full());
// 	EXPECT_FALSE(q.push(40)); // Fails because it's full

// 	EXPECT_EQ(q.peek().value(), 10);
// 	EXPECT_EQ(q[0], 10);
// 	EXPECT_EQ(q.at(1), 20);

// 	auto val1 = q.pop();
// 	EXPECT_TRUE(val1.has_value());
// 	EXPECT_EQ(val1.value(), 10);
// 	EXPECT_EQ(q.size(), 2);

// 	EXPECT_EQ(q.peek().value(), 20);

// 	q.clear();
// 	EXPECT_TRUE(q.empty());
// 	EXPECT_FALSE(q.pop().has_value());
// }

// TEST_F(ContainersTest, StaticQueue_CircularBehavior)
// {
// 	StaticQueue<int, 3> q;
// 	q.push(1);
// 	q.push(2);
// 	q.pop(); // head is now at index 1
// 	q.push(3);
// 	q.push(4); // tail is now at index 1

// 	EXPECT_EQ(q.size(), 3);
// 	EXPECT_TRUE(q.full());
// 	int expected = 2;
// 	for (const auto& i : q) {
// 		EXPECT_EQ(i, expected++);
// 	}

// 	EXPECT_EQ(q.pop().value(), 2);
// 	EXPECT_EQ(q.pop().value(), 3);
// 	EXPECT_EQ(q.pop().value(), 4);
// 	EXPECT_TRUE(q.empty());
// }

// TEST_F(ContainersTest, StaticQueue_WithTestCounter)
// {
// 	{
// 		StaticQueue<TestCounter, 5> q;
// 		q.emplace(1);
// 		q.push(TestCounter(2));
// 		TestCounter tmp(3);
// 		q.push(tmp);

// 		EXPECT_EQ(q.size(), 3);
// 		EXPECT_EQ(TestCounter::constructions, 5); // 1 emplace, 2 push move, 2 push copy
// 		int expected = 1;
// 		for (const auto& ctr : q) {
// 			EXPECT_EQ(ctr.value, expected++);
// 		}

// 		auto item = q.pop();
// 		EXPECT_EQ(item->value, 1);
// 		EXPECT_EQ(TestCounter::destructions, 3); // 1 push move, 2 pop (in store + tmp)
// 	}
// 	// All remaining items in queue are destroyed
// 	EXPECT_EQ(TestCounter::constructions, TestCounter::destructions);
// }

// TEST_F(ContainersTest, StaticQueue_CopyOperations_NoWrap)
// {
// 	StaticQueue<TestCounter, 5> original;
// 	original.emplace(1);
// 	original.emplace(2);
// 	original.emplace(3);
// 	TestCounter::reset();

// 	// Test Copy Constructor
// 	{
// 		StaticQueue<TestCounter, 5> copy = original;
// 		EXPECT_EQ(TestCounter::constructions, 3);
// 		EXPECT_EQ(TestCounter::copies, 3);
// 		EXPECT_EQ(TestCounter::moves, 0);
// 		EXPECT_EQ(copy.size(), original.size());
// 		for (size_t i = 0; i < copy.size(); ++i)
// 		{
// 			EXPECT_EQ(copy[i].value, original[i].value);
// 		}
// 	}
// 	EXPECT_EQ(TestCounter::destructions, 3);
// 	TestCounter::reset();

// 	// Test Copy Assignment
// 	{
// 		StaticQueue<TestCounter, 5> target;
// 		target.push(100);
// 		TestCounter::reset();

// 		target = original;
// 		EXPECT_EQ(TestCounter::constructions, 3);
// 		EXPECT_EQ(TestCounter::copies, 3);
// 		EXPECT_EQ(TestCounter::moves, 0);
// 		EXPECT_EQ(TestCounter::destructions, 1); // old item in target
// 		EXPECT_EQ(target.size(), original.size());
// 		for (size_t i = 0; i < target.size(); ++i)
// 		{
// 			EXPECT_EQ(target[i].value, original[i].value);
// 		}
// 	}
// }

// TEST_F(ContainersTest, StaticQueue_CopyOperations_WithWrap)
// {
// 	StaticQueue<TestCounter, 4> original;
// 	original.emplace(1);
// 	original.emplace(2);
// 	original.emplace(3);
// 	original.emplace(4);
// 	original.pop(); // val 1
// 	original.pop(); // val 2. head is at index 2. size is 2.
// 	original.emplace(5); // val 5
// 	original.emplace(6); // val 6. queue is {3, 4, 5, 6}
// 	TestCounter::reset();

// 	// Test Copy Constructor
// 	{
// 		StaticQueue<TestCounter, 4> copy = original;
// 		EXPECT_EQ(TestCounter::constructions, 4);
// 		EXPECT_EQ(TestCounter::copies, 4);
// 		EXPECT_EQ(TestCounter::moves, 0);
// 		EXPECT_EQ(copy.size(), original.size());
// 		for (size_t i = 0; i < original.size(); ++i)
// 		{
// 			EXPECT_EQ(copy.pop()->value, original[i].value);
// 		}
// 		EXPECT_EQ(original.size(), 4);
// 		EXPECT_EQ(copy.size(), 0);
// 	}
// 	TestCounter::reset();

// 	// Test Copy Assignment
// 	{
// 		StaticQueue<TestCounter, 4> target;
// 		target.push(100);
// 		TestCounter::reset();

// 		target = original;
// 		EXPECT_EQ(TestCounter::constructions, 4);
// 		EXPECT_EQ(TestCounter::copies, 4);
// 		EXPECT_EQ(TestCounter::moves, 0);
// 		EXPECT_EQ(TestCounter::destructions, 1); // old item in target
// 		EXPECT_EQ(target.size(), original.size());
// 		for (size_t i = 0; i < target.size(); ++i)
// 		{
// 			EXPECT_EQ(target[i].value, original[i].value);
// 		}
// 	}
// }

// TEST_F(ContainersTest, StaticQueue_MoveOperations_NoWrap)
// {
// 	// Test Move Constructor
// 	{
// 		StaticQueue<TestCounter, 5> original;
// 		original.emplace(1);
// 		original.emplace(2);
// 		original.emplace(3);
// 		TestCounter::reset();

// 		StaticQueue<TestCounter, 5> moved = std::move(original);
// 		EXPECT_EQ(TestCounter::constructions, 3);
// 		EXPECT_EQ(TestCounter::copies, 0);
// 		EXPECT_EQ(TestCounter::moves, 3);
// 		EXPECT_EQ(moved.size(), 3);
// 		EXPECT_EQ(original.size(), 0);
// 		EXPECT_EQ(moved.pop()->value, 1);
// 		EXPECT_EQ(moved.pop()->value, 2);
// 		EXPECT_EQ(moved.pop()->value, 3);
// 	}
// 	TestCounter::reset();

// 	// Test Move Assignment
// 	{
// 		StaticQueue<TestCounter, 5> original;
// 		original.emplace(1);
// 		original.emplace(2);
// 		original.emplace(3);
// 		StaticQueue<TestCounter, 5> target;
// 		target.emplace(100);
// 		TestCounter::reset();

// 		target = std::move(original);
// 		EXPECT_EQ(TestCounter::constructions, 3);
// 		EXPECT_EQ(TestCounter::copies, 0);
// 		EXPECT_EQ(TestCounter::moves, 3);
// 		EXPECT_EQ(TestCounter::destructions, 4);
// 		EXPECT_EQ(target.size(), 3);
// 		EXPECT_EQ(original.size(), 0);
// 		EXPECT_EQ(target.pop()->value, 1);
// 	}
// }

// TEST_F(ContainersTest, StaticQueue_MoveOperations_WithWrap)
// {
// 	// Test Move Constructor
// 	{
// 		StaticQueue<TestCounter, 5> original;
// 		original.emplace(1);
// 		original.emplace(2);
// 		original.pop();
// 		original.emplace(3);
// 		original.emplace(4);
// 		original.emplace(5); // queue is {2, 3, 4, 5} wrapped
// 		TestCounter::reset();

// 		StaticQueue<TestCounter, 5> moved = std::move(original);
// 		EXPECT_EQ(TestCounter::constructions, 4);
// 		EXPECT_EQ(TestCounter::copies, 0);
// 		EXPECT_EQ(TestCounter::moves, 4);
// 		EXPECT_EQ(moved.size(), 4);
// 		EXPECT_EQ(original.size(), 0);
// 		EXPECT_EQ(moved.pop()->value, 2);
// 		EXPECT_EQ(moved.pop()->value, 3);
// 		EXPECT_EQ(moved.pop()->value, 4);
// 		EXPECT_EQ(moved.pop()->value, 5);
// 	}
// 	TestCounter::reset();

// 	// Test Move Assignment
// 	{
// 		StaticQueue<TestCounter, 5> original;
// 		original.emplace(1);
// 		original.emplace(2);
// 		original.pop();
// 		original.emplace(3); // queue is {2, 3} wrapped
// 		StaticQueue<TestCounter, 5> target;
// 		target.emplace(100);
// 		TestCounter::reset();

// 		target = std::move(original);
// 		EXPECT_EQ(TestCounter::constructions, 2);
// 		EXPECT_EQ(TestCounter::copies, 0);
// 		EXPECT_EQ(TestCounter::moves, 2);
// 		EXPECT_EQ(TestCounter::destructions, 3);
// 		EXPECT_EQ(target.size(), 2);
// 		EXPECT_EQ(original.size(), 0);
// 		EXPECT_EQ(target.pop()->value, 2);
// 		EXPECT_EQ(target.pop()->value, 3);
// 	}
// }

// TEST_F(ContainersTest, StaticQueue_Swap_NoWrap)
// {
// 	StaticQueue<int, 5> q1;
// 	q1.push(1);
// 	q1.push(2);

// 	StaticQueue<int, 5> q2;
// 	q2.push(10);
// 	q2.push(20);
// 	q2.push(30);

// 	q1.swap(q2);

// 	EXPECT_EQ(q1.size(), 3);
// 	EXPECT_EQ(q2.size(), 2);

// 	EXPECT_EQ(q1.pop().value(), 10);
// 	EXPECT_EQ(q1.pop().value(), 20);
// 	EXPECT_EQ(q1.pop().value(), 30);

// 	EXPECT_EQ(q2.pop().value(), 1);
// 	EXPECT_EQ(q2.pop().value(), 2);
// }

// TEST_F(ContainersTest, StaticQueue_Swap_WithWrap)
// {
// 	StaticQueue<int, 4> q1;
// 	q1.push(1);
// 	q1.push(2);
// 	q1.push(3);
// 	q1.pop();
// 	q1.pop();
// 	q1.push(4);
// 	q1.push(5);
// 	q1.push(6);

// 	StaticQueue<int, 5> q2;
// 	q2.push(10);
// 	q2.push(20);

// 	q1.swap(q2);

// 	EXPECT_EQ(q1.size(), 2);
// 	EXPECT_EQ(q2.size(), 4);

// 	EXPECT_EQ(q1.pop().value(), 10);
// 	EXPECT_EQ(q1.pop().value(), 20);

// 	EXPECT_EQ(q2.pop().value(), 3);
// 	EXPECT_EQ(q2.pop().value(), 4);
// 	EXPECT_EQ(q2.pop().value(), 5);
// 	EXPECT_EQ(q2.pop().value(), 6);
// }

// TEST_F(ContainersTest, StaticQueue_Swap_DifferentCapacity)
// {
// 	StaticQueue<int, 5> q1;
// 	q1.push(1);
// 	q1.push(2);

// 	StaticQueue<int, 3> q2;
// 	q2.push(10);
// 	q2.push(20);
// 	q2.push(30);

// 	q1.swap(q2);

// 	EXPECT_EQ(q1.size(), 3);
// 	EXPECT_EQ(q2.size(), 2);

// 	EXPECT_EQ(q1.pop().value(), 10);
// 	EXPECT_EQ(q1.pop().value(), 20);
// 	EXPECT_EQ(q1.pop().value(), 30);

// 	EXPECT_EQ(q2.pop().value(), 1);
// 	EXPECT_EQ(q2.pop().value(), 2);
// }

// TEST_F(ContainersTest, StaticQueue_Swap_Throws)
// {
// 	StaticQueue<int, 2> q1;
// 	q1.push(1);

// 	StaticQueue<int, 5> q2;
// 	q2.push(10);
// 	q2.push(20);
// 	q2.push(30);

// 	// q2 has 3 elements, q1 has capacity 2. Should throw.
// 	EXPECT_THROW(q1.swap(q2), std::length_error);
// }

// TEST_F(ContainersTest, StaticQueue_InsertCopy_WithWrap)
// {
// 	StaticQueue<int, 5> q1;
// 	q1.push(0);
// 	q1.push(0);
// 	q1.pop();
// 	q1.pop();
// 	q1.push(1);
// 	q1.push(2);

// 	StaticQueue<int, 3> q2;
// 	q2.push(10);
// 	q2.push(20);
// 	q2.push(30);

// 	q1.insert(q1.end(), q2.begin(), q2.end());

// 	EXPECT_EQ(q1.size(), 5);
// 	EXPECT_EQ(q2.size(), 3);

// 	EXPECT_EQ(q1[0], 1);
// 	EXPECT_EQ(q1[1], 2);
// 	EXPECT_EQ(q1[2], 10);
// 	EXPECT_EQ(q1[3], 20);
// 	EXPECT_EQ(q1[4], 30);

// 	EXPECT_EQ(q2[0], 10);
// 	EXPECT_EQ(q2[1], 20);
// 	EXPECT_EQ(q2[2], 30);
// }

// TEST_F(ContainersTest, StaticQueue_InsertCopy_Throws)
// {
// 	StaticQueue<int, 5> q1;
// 	q1.push(1);
// 	q1.push(2);
// 	q1.push(3);

// 	StaticQueue<int, 5> q2;
// 	q2.push(10);
// 	q2.push(20);
// 	q2.push(30);

// 	EXPECT_THROW(q1.insert(q1.end(), q2.begin(), q2.end()), std::length_error);
// }

// TEST_F(ContainersTest, StaticQueue_InsertMove_NoWrap)
// {
// 	StaticQueue<int, 5> q1;
// 	q1.push(1);
// 	q1.push(2);

// 	StaticQueue<int, 5> q2;
// 	q2.push(10);
// 	q2.push(20);

// 	q1.insert(q1.end(), std::make_move_iterator(q2.begin()), std::make_move_iterator(q2.end()));

// 	EXPECT_EQ(q1.size(), 4);
// 	EXPECT_EQ(q2.size(), 2);

// 	EXPECT_EQ(q1[0], 1);
// 	EXPECT_EQ(q1[1], 2);
// 	EXPECT_EQ(q1[2], 10);
// 	EXPECT_EQ(q1[3], 20);
// }