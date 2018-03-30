#include "gtest/gtest.h"
#include "test/test_utils.h"

GTEST_API_ int main(int argc, char** argv) {
  Lotus::Test::DefaultInitialize(argc, argv);
  
  return RUN_ALL_TESTS();
}
