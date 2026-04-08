#include "external/json.hpp"
#include "sun_position_evaluator.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using json = nlohmann::ordered_json;

namespace {

struct CliArguments {
    std::string m_input_file_path;
    std::string m_output_file_path;
};

CliArguments parse_command_line_arguments(int argc, char* argv[])
{
    CliArguments arguments{};

    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];

        if (argument == "--input") {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing value after --input.");
            }
            arguments.m_input_file_path = argv[++i];
        }
        else if (argument == "--output") {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing value after --output.");
            }
            arguments.m_output_file_path = argv[++i];
        }
        else {
            throw std::runtime_error("Unknown command-line argument: " + argument);
        }
    }

    if (arguments.m_input_file_path.empty() ||
        arguments.m_output_file_path.empty()) {
        throw std::runtime_error(
            "Usage: sun_position_evaluator --input input.json --output output.json");
    }

    return arguments;
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

void write_json_file(const fs::path& file_path, const json& output_json)
{
    std::ofstream output_file(file_path);
    if (!output_file.is_open()) {
        throw std::runtime_error("Failed to open output JSON file: " + file_path.string());
    }

    output_file << std::setw(4) << output_json << '\n';
}

fs::path find_evaluator_spec_file()
{
    const fs::path current_working_directory = fs::current_path();
    const fs::path evaluator_spec_file_path =
        current_working_directory / "sun_position_evaluator_spec.json";

    if (!fs::exists(evaluator_spec_file_path)) {
        throw std::runtime_error(
            "Could not find evaluator_spec.json in working directory: " +
            current_working_directory.string());
    }

    return evaluator_spec_file_path;
}

double parse_named_parameter(
    const json& parameter_container,
    const std::string& parameter_name)
{
    if (!parameter_container.contains(parameter_name)) {
        throw std::runtime_error(
            "Missing parameter '" + parameter_name + "'.");
    }

    return parameter_container.at(parameter_name).get<double>();
}

SunPositionAlgorithm::ParameterVector parse_parameter_vector(const json& input_json)
{
    SunPositionAlgorithm::ParameterVector parameters{};

    if (input_json.contains("parameters")) {
        const json& parameters_json = input_json.at("parameters");

        if (parameters_json.is_array()) {
            if (parameters_json.size() != 15) {
                throw std::runtime_error(
                    "'parameters' must contain exactly 15 values.");
            }

            for (std::size_t i = 0; i < parameters.size(); ++i) {
                parameters[i] = parameters_json.at(i).get<double>();
            }

            return parameters;
        }

        if (parameters_json.is_object()) {
            for (std::size_t i = 0; i < parameters.size(); ++i) {
                const std::string parameter_name = "p" + std::to_string(i);
                parameters[i] = parse_named_parameter(parameters_json, parameter_name);
            }

            return parameters;
        }

        throw std::runtime_error(
            "'parameters' must be either an array or an object.");
    }

    bool has_top_level_named_parameters = true;
    for (std::size_t i = 0; i < parameters.size(); ++i) {
        const std::string parameter_name = "p" + std::to_string(i);
        if (!input_json.contains(parameter_name)) {
            has_top_level_named_parameters = false;
            break;
        }
    }

    if (has_top_level_named_parameters) {
        for (std::size_t i = 0; i < parameters.size(); ++i) {
            const std::string parameter_name = "p" + std::to_string(i);
            parameters[i] = input_json.at(parameter_name).get<double>();
        }

        return parameters;
    }

    throw std::runtime_error(
        "Could not find evaluator parameters. Expected one of:\n"
        "  1. parameters: [15 values]\n"
        "  2. parameters: {p0, ..., p14}\n"
        "  3. top-level keys p0, ..., p14");
}

fs::path parse_mica_binary_file_path(
    const json& evaluator_spec_json,
    const fs::path& evaluator_spec_file_path)
{
    if (!evaluator_spec_json.contains("mica_binary_file_path")) {
        throw std::runtime_error(
            "evaluator_spec.json does not contain 'mica_binary_file_path'.");
    }

    const fs::path raw_path =
        evaluator_spec_json.at("mica_binary_file_path").get<std::string>();

    if (raw_path.is_absolute()) {
        return raw_path;
    }

    const fs::path evaluator_spec_directory = evaluator_spec_file_path.parent_path();
    return fs::weakly_canonical(evaluator_spec_directory / raw_path);
}

GeoLocation parse_location(const json& evaluator_spec_json)
{
    if (!evaluator_spec_json.contains("location")) {
        throw std::runtime_error(
            "evaluator_spec.json does not contain 'location'.");
    }

    const json& location_json = evaluator_spec_json.at("location");

    if (!location_json.contains("longitude")) {
        throw std::runtime_error(
            "evaluator_spec.json location does not contain 'longitude'.");
    }

    if (!location_json.contains("latitude")) {
        throw std::runtime_error(
            "evaluator_spec.json location does not contain 'latitude'.");
    }

    GeoLocation location{};
    location.longitude = location_json.at("longitude").get<double>();
    location.latitude = location_json.at("latitude").get<double>();

    return location;
}

json build_error_statistics_json(const ErrorStatistics& statistics)
{
    json statistics_json;
    statistics_json["average"] = statistics.average;
    statistics_json["standard_deviation"] = statistics.standard_deviation;
    statistics_json["mean_deviation"] = statistics.mean_deviation;
    statistics_json["range"] = {statistics.minimum, statistics.maximum};
    return statistics_json;
}

json build_success_output_json(const EvaluationMetrics& metrics)
{
    json errors_json;
    errors_json["azimuth"] = build_error_statistics_json(metrics.azimuth_error_arcsec);
    errors_json["zenith"] = build_error_statistics_json(metrics.zenith_error_arcsec);
    errors_json["sun_vector"] = build_error_statistics_json(metrics.sun_vector_error_arcsec);

    json metrics_json;
    metrics_json["errors_arcsec"] = errors_json;
    metrics_json["sample_count"] = metrics.sample_count;

    json output_json;
    output_json["status"] = "success";
    output_json["objective"] = metrics.sun_vector_error_arcsec.average;
    output_json["metrics"] = metrics_json;

    return output_json;
}

json build_failure_output_json(const std::string& message)
{
    json output_json;
    output_json["status"] = "failure";
    output_json["error_type"] = "runtime_error";
    output_json["message"] = message;
    return output_json;
}

} // namespace

int main(int argc, char* argv[])
{
    try {
        const CliArguments arguments = parse_command_line_arguments(argc, argv);

        const fs::path input_file_path(arguments.m_input_file_path);
        const fs::path output_file_path(arguments.m_output_file_path);

        const json input_json = read_json_file(input_file_path);

        const fs::path evaluator_spec_file_path = find_evaluator_spec_file();
        const json evaluator_spec_json = read_json_file(evaluator_spec_file_path);

        const SunPositionAlgorithm::ParameterVector parameters =
            parse_parameter_vector(input_json);

        const fs::path mica_binary_file_path =
            parse_mica_binary_file_path(evaluator_spec_json, evaluator_spec_file_path);

        const GeoLocation location =
            parse_location(evaluator_spec_json);

        SunPositionEvaluator evaluator(mica_binary_file_path.string(), location);
        const EvaluationMetrics metrics = evaluator.evaluate(parameters);

        const json output_json = build_success_output_json(metrics);
        write_json_file(output_file_path, output_json);

        return 0;
    }
    catch (const std::exception& exception) {
        try {
            const CliArguments arguments = parse_command_line_arguments(argc, argv);
            const fs::path output_file_path(arguments.m_output_file_path);
            const json output_json = build_failure_output_json(exception.what());
            write_json_file(output_file_path, output_json);
        }
        catch (...) {
            std::cerr << "Error: " << exception.what() << '\n';
        }

        return 1;
    }
}