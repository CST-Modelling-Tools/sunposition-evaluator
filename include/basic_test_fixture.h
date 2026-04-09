#ifndef BASIC_TEST_FIXTURE_H
#define BASIC_TEST_FIXTURE_H

#include "sun_position_algorithm.h"

#include <cstdint>
#include <filesystem>

struct BasicTestFixture {
    std::filesystem::path m_fixture_directory;
    std::filesystem::path m_spec_file_path;
    std::filesystem::path m_input_file_path;
    std::filesystem::path m_csv_file_path;
    std::filesystem::path m_binary_file_path;
    GeoLocation m_location{};
    SunPositionAlgorithm::ParameterVector m_parameters{};
    std::uint64_t m_expected_sample_count = 0;
};

BasicTestFixture load_basic_test_fixture(
    const std::filesystem::path& fixture_directory);

void ensure_basic_test_binary_exists(const BasicTestFixture& fixture);

std::filesystem::path resolve_basic_test_fixture_directory(
    const std::filesystem::path& fixture_directory);

#endif // BASIC_TEST_FIXTURE_H
