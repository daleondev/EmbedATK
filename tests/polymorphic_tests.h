#pragma once

#include "EmbedATK/EmbedATK.h"
#include <gtest/gtest.h>

// Test Fixture
class PolymorphicTest : public ::testing::Test
{
protected:
	struct Base
	{
		static int construction_count;
		static int destruction_count;
		static int copy_construction_count;
		static int move_construction_count;

		int id;
		Base(int i) : id(i) { construction_count++; }
		virtual ~Base() { destruction_count++; }
		Base(const Base& other) : id(other.id)
		{
			construction_count++;
			copy_construction_count++;
		}
		Base(Base&& other) : id(other.id)
		{
			construction_count++;
			move_construction_count++;
		}

		virtual int value() const = 0;

		static void reset_counts()
		{
			construction_count = 0;
			destruction_count = 0;
			copy_construction_count = 0;
			move_construction_count = 0;
		}
	};

	struct Derived1 : public Base
	{
		int val;
		Derived1(int i, int v) : Base(i), val(v) {}
		int value() const override { return val; }
	};

	struct Derived2 : public Base
	{
		double val;
		Derived2(int i, double v) : Base(i), val(v) {}
		int value() const override { return static_cast<int>(val); }
	};

	// Sized to fit the larger of the two derived types
	static constexpr auto alloc = AllocData{
		.size = std::max(sizeof(Derived1), sizeof(Derived2)),
		.align = std::max(alignof(Derived1), alignof(Derived2))};
	using TestStaticPolymorphic = StaticPolymorphicStore<Base, alloc>;
	using TestStaticPolymorphicWithDerived = StaticPolymorphic<Base, std::tuple<Derived1, Derived2>>;

	void SetUp() override { Base::reset_counts(); }
};

int PolymorphicTest::Base::construction_count = 0;
int PolymorphicTest::Base::destruction_count = 0;
int PolymorphicTest::Base::copy_construction_count = 0;
int PolymorphicTest::Base::move_construction_count = 0;

//------------------------------------------------------
//               StaticPolymorphicStore
//------------------------------------------------------

TEST_F(PolymorphicTest, Static_DefaultConstruction)
{
	TestStaticPolymorphic p;
	EXPECT_FALSE(p);
	EXPECT_EQ(p.get(), nullptr);
	EXPECT_EQ(p.type(), typeid(void));
}

TEST_F(PolymorphicTest, Static_Construct)
{
	TestStaticPolymorphic p;
	p.construct<Derived1>(1, 100);

	EXPECT_TRUE(p);
	ASSERT_NE(p.get(), nullptr);
	EXPECT_EQ(p.get()->id, 1);
	EXPECT_EQ(p.get()->value(), 100);
	EXPECT_EQ(Base::construction_count, 1);
	EXPECT_EQ(p.type(), typeid(Derived1));

	// as<>()
	auto* as_ptr = p.as<Derived1>();
	ASSERT_NE(as_ptr, nullptr);
	EXPECT_EQ(as_ptr->val, 100);

	// cast<>()
	auto* cast_ptr1 = p.cast<Derived1>();
	ASSERT_NE(cast_ptr1, nullptr);
	EXPECT_EQ(cast_ptr1->val, 100);

	auto* cast_ptr2 = p.cast<Derived2>();
	EXPECT_EQ(cast_ptr2, nullptr);
}

TEST_F(PolymorphicTest, Static_Destroy)
{
	TestStaticPolymorphic p;
	p.construct<Derived1>(1, 100);
	ASSERT_EQ(Base::construction_count, 1);
	ASSERT_EQ(Base::destruction_count, 0);

	p.destroy();
	EXPECT_FALSE(p);
	EXPECT_EQ(p.get(), nullptr);
	EXPECT_EQ(Base::destruction_count, 1);
	EXPECT_EQ(p.type(), typeid(void));
}

TEST_F(PolymorphicTest, Static_Reconstruct)
{
	TestStaticPolymorphic p;
	p.construct<Derived1>(1, 100);
	ASSERT_EQ(Base::construction_count, 1);
	ASSERT_EQ(p.type(), typeid(Derived1));

	p.construct<Derived2>(2, 200.5);
	EXPECT_TRUE(p);
	ASSERT_NE(p.get(), nullptr);
	EXPECT_EQ(p.get()->id, 2);
	EXPECT_EQ(p.get()->value(), 200);
	EXPECT_EQ(Base::construction_count, 2); // new object constructed
	EXPECT_EQ(Base::destruction_count, 1); // old object destroyed
	EXPECT_EQ(p.type(), typeid(Derived2));
}

TEST_F(PolymorphicTest, Static_MoveConstruction)
{
	TestStaticPolymorphic p1;
	p1.construct<Derived1>(1, 100);
	Base::reset_counts();

	TestStaticPolymorphic p2(std::move(p1));
	EXPECT_EQ(Base::construction_count, 0); // No new object, just memory move
	EXPECT_EQ(Base::destruction_count, 0);

	EXPECT_FALSE(p1); // p1 is now invalid
	EXPECT_TRUE(p2);
	ASSERT_NE(p2.get(), nullptr);
	EXPECT_EQ(p2.get()->id, 1);
	EXPECT_EQ(p2.get()->value(), 100);
}

TEST_F(PolymorphicTest, Static_MoveAssignment)
{
	TestStaticPolymorphic p1;
	p1.construct<Derived1>(1, 100);
	TestStaticPolymorphic p2;
	p2.construct<Derived2>(2, 200.5);
	Base::reset_counts();

	p2 = std::move(p1);
	EXPECT_EQ(Base::destruction_count, 1); // p2's old object destroyed
	EXPECT_EQ(Base::construction_count, 0); // No new object

	EXPECT_FALSE(p1);
	EXPECT_TRUE(p2);
	ASSERT_NE(p2.get(), nullptr);
	EXPECT_EQ(p2.get()->id, 1);
	EXPECT_EQ(p2.get()->value(), 100);
}

TEST_F(PolymorphicTest, Static_Clone)
{
	TestStaticPolymorphic p1;
	p1.construct<Derived1>(1, 100);
	TestStaticPolymorphic p2;
	Base::reset_counts();

	p2.clone(*p1.as<Derived1>());
	EXPECT_EQ(Base::copy_construction_count, 1);
	EXPECT_TRUE(p2);
	ASSERT_NE(p2.get(), nullptr);
	EXPECT_NE(p1.get(), p2.get());
	EXPECT_EQ(p2.get()->id, 1);
	EXPECT_EQ(p2.get()->value(), 100);
}

TEST_F(PolymorphicTest, Static_With_Derived)
{
	TestStaticPolymorphicWithDerived p;
	ASSERT_EQ(p.size(), std::max(sizeof(Derived1), sizeof(Derived2)));

	p.construct<Derived1>(1, 100);

	EXPECT_TRUE(p);
	ASSERT_NE(p.get(), nullptr);
	EXPECT_EQ(p.get()->id, 1);
	EXPECT_EQ(p.get()->value(), 100);
	EXPECT_EQ(Base::construction_count, 1);
	EXPECT_EQ(p.type(), typeid(Derived1));

	// as<>()
	auto* as_ptr = p.as<Derived1>();
	ASSERT_NE(as_ptr, nullptr);
	EXPECT_EQ(as_ptr->val, 100);

	// cast<>()
	auto* cast_ptr1 = p.cast<Derived1>();
	ASSERT_NE(cast_ptr1, nullptr);
	EXPECT_EQ(cast_ptr1->val, 100);

	auto* cast_ptr2 = p.cast<Derived2>();
	EXPECT_EQ(cast_ptr2, nullptr);
}

//------------------------------------------------------
//                 DynamicPolymorphic
//------------------------------------------------------

TEST_F(PolymorphicTest, Dynamic_DefaultConstruction)
{
	DynamicPolymorphic<Base> p;
	EXPECT_FALSE(p);
	EXPECT_EQ(p.get(), nullptr);
	EXPECT_EQ(p.type(), typeid(void));
}

TEST_F(PolymorphicTest, Dynamic_Construct)
{
	DynamicPolymorphic<Base> p;
	p.construct<Derived1>(1, 100);

	EXPECT_TRUE(p);
	ASSERT_NE(p.get(), nullptr);
	EXPECT_EQ(p.get()->id, 1);
	EXPECT_EQ(p.get()->value(), 100);
	EXPECT_EQ(Base::construction_count, 1);
	EXPECT_EQ(p.type(), typeid(Derived1));
}

TEST_F(PolymorphicTest, Dynamic_Destroy)
{
	DynamicPolymorphic<Base> p;
	p.construct<Derived1>(1, 100);
	Base::reset_counts();

	p.destroy();
	EXPECT_FALSE(p);
	EXPECT_EQ(p.get(), nullptr);
	EXPECT_EQ(Base::destruction_count, 1);
	EXPECT_EQ(p.type(), typeid(void));
}

TEST_F(PolymorphicTest, Dynamic_MoveConstruction)
{
    DynamicPolymorphic<Base> p1;
    p1.construct<Derived1>(1, 100);
    auto* original_ptr = p1.get();
    Base::reset_counts();

    DynamicPolymorphic p2(std::move(p1));
    EXPECT_EQ(Base::construction_count, 0);
    EXPECT_EQ(Base::destruction_count, 0);

    EXPECT_FALSE(p1);
    EXPECT_TRUE(p2);
    EXPECT_EQ(p2.get(), original_ptr);
}
