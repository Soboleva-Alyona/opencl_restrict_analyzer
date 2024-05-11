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
TEST(TestRaces, test_benign_write_zero) {
    EXPECT_TRUE(analyze_races(test_races_fail_file_path, "test_benign_write_zero").empty());
}

TEST(TestRaces, test_benign_write_zero_local) {
    EXPECT_TRUE(analyze_races(test_races_fail_file_path, "test_benign_write_zero_local").empty());
}

TEST(TestRaces, test_index_local_id) {
    EXPECT_TRUE(analyze_races(test_races_path_file_path, "test_index_local_id").empty());
}

TEST(TestRaces, test_private_var) {
    EXPECT_TRUE(analyze_races(test_races_path_file_path, "test_private_var").empty());
}

TEST(TestRaces, test_private_arr) {
    EXPECT_TRUE(analyze_races(test_races_path_file_path, "test_private_arr").empty());
}

TEST(TestRaces, test_index_local_id_combination_pass) {
    EXPECT_TRUE(analyze_races(test_races_path_file_path, "test_index_local_id_combination_pass").empty());
}

TEST(TestRaces, test_assign_right_part_read_racy) {
    EXPECT_TRUE(analyze_races(test_races_path_file_path, "test_assign_right_part_read_racy").empty());
}

TEST(TestRaces, test_assign_same_addr_same_value) {
    EXPECT_TRUE(analyze_races(test_races_path_file_path, "test_assign_same_addr_same_value").empty());
}

TEST(TestRaces, test_assign_same_value_with_local_id) {
    EXPECT_TRUE(analyze_races(test_races_path_file_path, "test_assign_same_value_with_local_id").empty());
}

TEST(TestRaces, test_barrier_between_writes) {
    EXPECT_TRUE(analyze_races(test_races_path_file_path, "test_barrier_between_writes").empty());
}

TEST(TestRaces, test_barrier_for_local) {
    EXPECT_TRUE(analyze_races(test_races_path_file_path, "test_barrier_for_local").empty());
}


// fails

TEST(TestRaces, test_index_local_id_combination) {
    EXPECT_FALSE(analyze_races(test_races_fail_file_path, "test_index_local_id_combination").empty());
}

TEST(TestRaces, test_local_arr_declaration_to_int_index) {
    EXPECT_FALSE(analyze_races(test_races_fail_file_path, "test_local_arr_declaration_to_int_index").empty());
}

TEST(TestRaces, test_local_arr_declaration) {
    EXPECT_FALSE(analyze_races(test_races_fail_file_path, "test_local_arr_declaration").empty());
}

TEST(TestRaces, test_index_global_id_combination) {
    EXPECT_FALSE(analyze_races(test_races_fail_file_path, "test_index_global_id_combination").empty());
}

TEST(TestRaces, test_index_local_id_mod_local_size) {
    EXPECT_FALSE(analyze_races(test_races_fail_file_path, "test_index_local_id_mod_local_size").empty());
}

TEST(TestRaces, test_assign_same_addr_diff_value) {
    EXPECT_FALSE(analyze_races(test_races_fail_file_path, "test_assign_same_addr_diff_value").empty());
}

TEST(TestRaces, test_no_barrier_between_writes) {
    EXPECT_FALSE(analyze_races(test_races_fail_file_path, "test_no_barrier_between_writes").empty());
}

TEST(TestRaces, test_wrong_barrier_between_writes) {
    EXPECT_FALSE(analyze_races(test_races_fail_file_path, "test_wrong_barrier_between_writes").empty());
}

TEST(TestRaces, test_no_barrier_for_local) {
    EXPECT_FALSE(analyze_races(test_races_fail_file_path, "test_no_barrier_for_local").empty());
}

TEST(TestRaces, PipeCNN)
{
    EXPECT_EQ(
        analyze_pipe_cnn(test_pipe_cnn_file_path,"lrn",clsa::analyzer::checks::race).size(),
        16
    );
}
