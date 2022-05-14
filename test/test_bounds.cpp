#include <gtest/gtest.h>

#include <test_resources.h>
#include "test_utils.h"

#include "../lib/analyzer/frontend/analyzer.h"

std::vector<clsa::violation> analyze_bounds(std::string_view kernel) {
    return analyze(test_bounds_file_path, kernel, clsa::analyzer::checks::bounds);
}

TEST(TestBounds, NoViolationTrivial) {
    EXPECT_TRUE(analyze_bounds("test_no_violation_trivial").empty());
}

TEST(TestBounds, ViolationTrivial) {
    EXPECT_FALSE(analyze_bounds("test_violation_trivial").empty());
}

TEST(TestBounds, NoViolationRelative) {
    EXPECT_TRUE(analyze_bounds("test_no_violation_relative").empty());
}

TEST(TestBounds, ViolationRelative) {
    EXPECT_FALSE(analyze_bounds("test_violation_relative").empty());
}

TEST(TestBounds, ViolationInsideLoop) {
    EXPECT_FALSE(analyze_bounds("test_violation_inside_loop").empty());
}

TEST(TestBounds, NoViolationInsideLoop) {
    EXPECT_TRUE(analyze_bounds("test_no_violation_inside_loop").empty());
}
