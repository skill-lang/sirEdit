#include <gtest/gtest.h>
#include <sirEdit/data/types.hpp>

using namespace sirEdit::data;

TEST(rule_3, ReadLowerWrite) {
	EXPECT_LT(FIELD_STATE::READ, FIELD_STATE::WRITE);
}
TEST(rule_3, WriteLowerCreate) {
	EXPECT_LT(FIELD_STATE::WRITE, FIELD_STATE::CREATE);
}
