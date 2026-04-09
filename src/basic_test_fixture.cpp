#include "basic_test_fixture.h"

#include "external/json.hpp"
#include "mica_binary_dataset_builder.h"

#include <fstream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using json = nlohmann::ordered_json;

namespace {

fs::path normalize_path(const fs::path& path)
{
    std::error_code error_code;
    const fs::path canonical_path = fs::weakly_canonical(path, error_code);
    if (!error_code) {
        return canonical_path;
    }

    return fs::absolute(path).lexically_normal();
}

fs::path resolve_path_from_base(
    const fs::path& raw_path,
    const fs::path& base_directory)
{
    if (raw_path.is_absolute()) {
        return normalize_path(raw_path);
    }

    return normalize_path(base_directory / raw_path);
}

json read_json_file(const fs::path& file_path)
{
    std::ifstream input_file(file_path);
    if (!input_file.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + file_path.string());
    }

    json input_json;
    input_file >> input_json;
    return input_json;
}

std::uint64_t count_non_empty_lines(const fs::path& file_path)
{
    std::ifstream input_file(file_path);
    if (!input_file.is_open()) {
        throw std::runtime_error("Failed to open CSV file: " + file_path.string());
    }

    std::uint64_t line_count = 0;
    std::string line;
    while (std::getline(input_file, line)) {
        if (!line.empty()) {
            ++line_count;
        }
    }

    return line_count;
}

SunPositionAlgorithm::ParameterVector parse_parameter_vector(const json& input_json)
{
    SunPositionAlgorithm::ParameterVector parameters{};

    if (input_json.contains("parameters")) {
        const json& parameters_json = input_json.at("parameters");

        if (!parameters_json.is_array() || parameters_json.size() != parameters.size()) {
            throw std::runtime_error(
                "'parameters' must contain exactly 15 values.");
        }

        for (std::size_t i = 0; i < parameters.size(); ++i) {
            parameters[i] = parameters_json.at(i).get<double>();
        }

        return parameters;
    }

    for (std::size_t i = 0; i < parameters.size(); ++i) {
        const std::string parameter_name = "p" + std::to_string(i);
        if (!input_json.contains(parameter_name)) {
            throw std::runtime_error(
                "Missing parameter '" + parameter_name + "'.");
        }

        parameters[i] = input_json.at(parameter_name).get<double>();
    }

    return parameters;
}

GeoLocation parse_location(const json& spec_json)
{
    if (!spec_json.contains("location")) {
        throw std::runtime_error(
            "Fixture spec does not contain 'location'.");
    }

    const json& location_json = spec_json.at("location");

    GeoLocation location{};
    location.longitude = location_json.at("longitude").get<double>();
    location.latitude = location_json.at("latitude").get<double>();
    return location;
}

fs::path parse_binary_path(
    const json& spec_json,
    const fs::path& spec_file_path)
{
    if (!spec_json.contains("mica_binary_file_path")) {
        throw std::runtime_error(
            "Fixture spec does not contain 'mica_binary_file_path'.");
    }

    const fs::path raw_path =
        spec_json.at("mica_binary_file_path").get<std::string>();

    if (raw_path.is_absolute()) {
        return normalize_path(raw_path);
    }

    return resolve_path_from_base(raw_path, spec_file_path.parent_path());
}

} // namespace

fs::path resolve_basic_test_fixture_directory(const fs::path& fixture_directory)
{
    const fs::path direct_candidate = normalize_path(fixture_directory);
    if (fs::exists(direct_candidate)) {
        return direct_candidate;
    }

    #ifdef SUNPOSITION_SOURCE_DIR
    const fs::path source_tree_candidate =
        normalize_path(fs::path(SUNPOSITION_SOURCE_DIR) / fixture_directory);
    if (fs::exists(source_tree_candidate)) {
        return source_tree_candidate;
    }
    #endif

    throw std::runtime_error(
        "Fixture directory does not exist: " +
        normalize_path(fixture_directory).string());
}

BasicTestFixture load_basic_test_fixture(const fs::path& fixture_directory)
{
    BasicTestFixture fixture{};
    fixture.m_fixture_directory =
        resolve_basic_test_fixture_directory(fixture_directory);
    fixture.m_spec_file_path =
        fixture.m_fixture_directory / "sun_position_evaluator_spec.json";
    fixture.m_input_file_path =
        fixture.m_fixture_directory / "default_psa_plus_input.json";
    fixture.m_csv_file_path =
        fixture.m_fixture_directory / "data" / "MICA_psa_2020_2050_MB.csv";

    const json spec_json = read_json_file(fixture.m_spec_file_path);
    const json input_json = read_json_file(fixture.m_input_file_path);

    fixture.m_binary_file_path =
        parse_binary_path(spec_json, fixture.m_spec_file_path);
    fixture.m_location = parse_location(spec_json);
    fixture.m_parameters = parse_parameter_vector(input_json);
    fixture.m_expected_sample_count = count_non_empty_lines(fixture.m_csv_file_path);

    return fixture;
}

void ensure_basic_test_binary_exists(const BasicTestFixture& fixture)
{
    if (fs::exists(fixture.m_binary_file_path)) {
        return;
    }

    build_mica_binary_dataset(
        fixture.m_csv_file_path,
        fixture.m_binary_file_path);
}
