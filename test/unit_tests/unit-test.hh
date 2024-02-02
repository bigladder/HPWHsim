/*
 * HPWHsim unit tests
 */

#ifndef HPWH_UNIT_TEST_hh
#define HPWH_UNIT_TEST_hh

// vendor
#include <fmt/format.h>
#include <gtest/gtest.h>

#define EXPECT_NEAR_REL_TOL(A, B, eps) EXPECT_NEAR(A, B, eps*(abs(A) < abs(B) ? abs(B) : abs(A)))

#define EXPECT_NEAR_REL(A, B) EXPECT_NEAR_REL_TOL(A, B, 1.e-4)

#define EXPECT_FAR(A, B, eps) EXPECT_FALSE(abs(A - B) < eps)

#endif
