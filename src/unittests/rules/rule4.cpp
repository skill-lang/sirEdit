#include <gtest/gtest.h>
#include <sirEdit/data/types.hpp>

using namespace sirEdit::data;

TEST(rule_4, ReadLowerWrite) {
	EXPECT_LT(TYPE_STATE::READ, TYPE_STATE::WRITE);
}
TEST(rule_4, WriteLowerDelete) {
	EXPECT_LT(TYPE_STATE::WRITE, TYPE_STATE::DELETE);
}
