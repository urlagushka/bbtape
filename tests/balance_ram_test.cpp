#include <gtest/gtest.h>

#include <bbtape/ram_handler.hpp>

/*
я сделал один тест и ушел спать
времени мало осталось, а у меня еще телеком
поймите пожалуйста
*/

TEST(balance_ram_test, zero_lhs_size)
{
  EXPECT_EQ(bb::balance_ram_block(100, 0, 50), 0);
}

TEST(balance_ram_test, zero_rhs_size)
{
  EXPECT_EQ(bb::balance_ram_block(100, 50, 0), 100);
}

TEST(balance_ram_test, lhs_rhs_fit_in_ram)
{
  EXPECT_EQ(bb::balance_ram_block(100, 30, 40), 30);
}

TEST(balance_ram_test, lhs_rhs_exceed_ram)
{
  EXPECT_EQ(bb::balance_ram_block(100, 70, 30), 70);
  EXPECT_EQ(bb::balance_ram_block(100, 30, 70), 30);
}

TEST(balance_ram_test, min_size_constraint)
{
  EXPECT_EQ(bb::balance_ram_block(10, 1, 100), 1);
}

TEST(balance_ram_test, max_size_constraint)
{
  EXPECT_EQ(bb::balance_ram_block(10, 100, 1), 9);
}
