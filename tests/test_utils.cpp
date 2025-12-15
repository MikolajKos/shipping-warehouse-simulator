#include <gtest/gtest.h>

// Code import
extern "C" {
  #include "../src/common/utils.h"
}

// Test example
TEST(UtilsTest, GeneratedWeightIsWithinBounds) {
  double weight = generate_weight(PKG_C);
  EXPECT_GE(weight, 0.1);  // Greater or Equal
  EXPECT_LE(weight, 25.0); // Less or Equal
}

TEST(UtilsTest, VolumeZeroForUnknownType) {
  double vol = get_volume((PackageType)999); 
  EXPECT_EQ(vol, 0.0);
}
