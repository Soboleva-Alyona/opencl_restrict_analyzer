#include <gtest/gtest.h>

#include <test_resources.h>
#include "test_utils.h"

#include "../lib/analyzer/frontend/analyzer.h"

std::vector<clsa::violation> analyze_races(std::string_view file_path, std::string_view kernel) {
    return analyze(file_path, kernel, clsa::analyzer::checks::race);
}

std::vector<clsa::violation> analyze_races(std::string_view kernel) {
    return analyze_races(test_bounds_file_path, kernel);
}

// pass
TEST(TestRaces, test_index_local_id) {
    EXPECT_TRUE(analyze_races(test_races_path_file_path, "test_index_local_id").empty());
}

TEST(TestRaces, test_private_var) {
    EXPECT_TRUE(analyze_races(test_races_path_file_path, "test_private_var").empty());
}

TEST(TestRaces, test_private_arr) {
    EXPECT_TRUE(analyze_races(test_races_path_file_path, "test_private_arr").empty());
}


// fails
TEST(TestRaces, test_write_zero) {
    EXPECT_FALSE(analyze_races(test_races_fail_file_path, "test_write_zero").empty());
}

TEST(TestRaces, test_write_zero_local) {
    EXPECT_FALSE(analyze_races(test_races_fail_file_path, "test_write_zero_local").empty());
}

TEST(TestRaces, test_index_local_id_combination) {
    EXPECT_FALSE(analyze_races(test_races_fail_file_path, "test_index_local_id_combination").empty());
}

TEST(TestRaces, test_index_parameter) {
    EXPECT_FALSE(analyze_races(test_races_fail_file_path, "test_write_zero_local").empty());
}

TEST(TestRaces, test_local_arr_declaration) {
    EXPECT_FALSE(analyze_races(test_races_fail_file_path, "test_write_zero_local").empty());
}
