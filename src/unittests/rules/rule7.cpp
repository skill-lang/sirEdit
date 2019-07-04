#include "model.hpp"
#include <gtest/gtest.h>
#include <sirEdit/data/tools.hpp>

using namespace sirEdit::data;

TEST(rule_7, DirectSubclass) {
	// Values
	TypeTestModel1 model;
	Tool t;

	// Set state test
	t.setTypeState(model.g, TYPE_STATE::READ);
	EXPECT_EQ(t.getTypeTransitiveState(model.g), TYPE_STATE::READ);
	EXPECT_EQ(t.getTypeTransitiveState(model.a), TYPE_STATE::READ);
	EXPECT_EQ(t.getTypeTransitiveState(model.i), TYPE_STATE::UNUSED);
	EXPECT_EQ(t.getTypeTransitiveState(model.b), TYPE_STATE::UNUSED);

	// Reset state
	t.setTypeState(model.g, TYPE_STATE::UNUSED);
	EXPECT_EQ(t.getTypeTransitiveState(model.g), TYPE_STATE::UNUSED);
	EXPECT_EQ(t.getTypeTransitiveState(model.a), TYPE_STATE::UNUSED);
	EXPECT_EQ(t.getTypeTransitiveState(model.i), TYPE_STATE::UNUSED);
	EXPECT_EQ(t.getTypeTransitiveState(model.b), TYPE_STATE::UNUSED);
}

TEST(rule_7, DirectInterface) {
	// Values
	TypeTestModel1 model;
	Tool t;

	// Set state test for b
	t.setTypeState(model.b, TYPE_STATE::READ);
	EXPECT_EQ(t.getTypeTransitiveState(model.b), TYPE_STATE::READ);
	EXPECT_EQ(t.getTypeTransitiveState(model.a), TYPE_STATE::READ);
	EXPECT_EQ(t.getTypeTransitiveState(model.c), TYPE_STATE::UNUSED);
	EXPECT_EQ(t.getTypeTransitiveState(model.i), TYPE_STATE::UNUSED);

	// Set state test for b and c
	// Additional cache poisen test
	t.setTypeState(model.c, TYPE_STATE::READ);
	EXPECT_EQ(t.getTypeTransitiveState(model.b), TYPE_STATE::READ);
	EXPECT_EQ(t.getTypeTransitiveState(model.a), TYPE_STATE::READ);
	EXPECT_EQ(t.getTypeTransitiveState(model.c), TYPE_STATE::READ);
	EXPECT_EQ(t.getTypeTransitiveState(model.i), TYPE_STATE::UNUSED);

	// Set state test for c
	// Addition cache poisen test
	t.setTypeState(model.b, TYPE_STATE::UNUSED);
	EXPECT_EQ(t.getTypeTransitiveState(model.b), TYPE_STATE::UNUSED);
	EXPECT_EQ(t.getTypeTransitiveState(model.a), TYPE_STATE::READ);
	EXPECT_EQ(t.getTypeTransitiveState(model.c), TYPE_STATE::READ);
	EXPECT_EQ(t.getTypeTransitiveState(model.i), TYPE_STATE::UNUSED);

	// Reset state
	// Addition cache poisen test
	t.setTypeState(model.c, TYPE_STATE::UNUSED);
	EXPECT_EQ(t.getTypeTransitiveState(model.b), TYPE_STATE::UNUSED);
	EXPECT_EQ(t.getTypeTransitiveState(model.a), TYPE_STATE::UNUSED);
	EXPECT_EQ(t.getTypeTransitiveState(model.c), TYPE_STATE::UNUSED);
	EXPECT_EQ(t.getTypeTransitiveState(model.i), TYPE_STATE::UNUSED);
}

TEST(rule_7, IndirectInterface) {
	// Values
	TypeTestModel1 model;
	Tool t;

	// Set state test for b
	t.setTypeState(model.d, TYPE_STATE::READ);
	EXPECT_EQ(t.getTypeTransitiveState(model.d), TYPE_STATE::READ);
	EXPECT_EQ(t.getTypeTransitiveState(model.a), TYPE_STATE::READ);
	EXPECT_EQ(t.getTypeTransitiveState(model.b), TYPE_STATE::READ);
	EXPECT_EQ(t.getTypeTransitiveState(model.c), TYPE_STATE::READ);
	EXPECT_EQ(t.getTypeTransitiveState(model.i), TYPE_STATE::UNUSED);

	// Reset
	t.setTypeState(model.d, TYPE_STATE::UNUSED);
	EXPECT_EQ(t.getTypeTransitiveState(model.d), TYPE_STATE::UNUSED);
	EXPECT_EQ(t.getTypeTransitiveState(model.a), TYPE_STATE::UNUSED);
	EXPECT_EQ(t.getTypeTransitiveState(model.b), TYPE_STATE::UNUSED);
	EXPECT_EQ(t.getTypeTransitiveState(model.c), TYPE_STATE::UNUSED);
	EXPECT_EQ(t.getTypeTransitiveState(model.i), TYPE_STATE::UNUSED);
}
