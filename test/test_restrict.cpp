#include <gtest/gtest.h>

#include <test_resources.h>
#include "test_utils.h"

#include "../lib/analyzer/frontend/analyzer.h"

std::vector<clsa::violation> analyze_restrict(std::string_view file_path, std::string_view kernel) {
    return analyze(file_path, kernel, clsa::analyzer::checks::restrict);
}

std::vector<clsa::violation> analyze_restrict(std::string_view kernel) {
    return analyze_restrict(test_restrict_file_path, kernel);
}

TEST(TestRestrict, NoViolationTrivial) {
    EXPECT_TRUE(analyze_restrict("test_no_violation_trivial").empty());
}

TEST(TestRestrict, ViolationTrivial) {
    EXPECT_FALSE(analyze_restrict("test_violation_trivial").empty());
}

TEST(TestRestrict, ViolationConditional) {
    EXPECT_FALSE(analyze_restrict("test_violation_conditional").empty());
}

TEST(TestRestrict, NoViolationUnreachableCondition) {
    EXPECT_TRUE(analyze_restrict("test_no_violation_unreachable_condition").empty());
}

TEST(TestRestrict, NoViolationUnreachableStatement) {
    EXPECT_TRUE(analyze_restrict("test_no_violation_unreachable_statement").empty());
}

TEST(TestRestrict, ViolationIncorrectFunctionCall) {
    EXPECT_FALSE(analyze_restrict("test_violation_incorrect_function_call").empty());
}

TEST(TestRestrict, NoViolationCorrectFunctionCall) {
    EXPECT_TRUE(analyze_restrict("test_no_violation_correct_function_call").empty());
}

TEST(TestRestrict, ViolationInsideLoop) {
    EXPECT_FALSE(analyze_restrict("test_violation_inside_loop").empty());
}

TEST(TestRestrict, NoViolationInsideLoopUnreachable) {
    EXPECT_TRUE(analyze_restrict("test_no_violation_inside_loop_unreachable").empty());
}

TEST(TestRestrict, ViolationDereference) {
    EXPECT_FALSE(analyze_restrict("test_violation_dereference").empty());
}

TEST(TestRestrict, PipeCNN_NoViolations) {
    EXPECT_TRUE(analyze_pipe_cnn(test_pipe_cnn_file_path, "lrn", clsa::analyzer::checks::restrict).empty());
}

TEST(TestRestrict, PipeCNN_Violations) {
    EXPECT_FALSE(analyze_pipe_cnn(test_pipe_cnn_file_path, "lrn_dirty", clsa::analyzer::checks::restrict).empty());
}

TEST(TestRestrict, oneDNN_NoViolations) {
    EXPECT_TRUE(analyze_one_dnn(test_one_dnn_file_path, "generic_reorder", clsa::analyzer::checks::restrict).empty());
}

TEST(TestRestrict, oneDNN_Violations) {
    EXPECT_FALSE(analyze_one_dnn(test_one_dnn_file_path, "generic_reorder_dirty", clsa::analyzer::checks::restrict).empty());
}
