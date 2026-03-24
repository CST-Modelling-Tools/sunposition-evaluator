#include "external/json.hpp"
#include "sun_position_evaluator.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

SunPositionAlgorithm::ParameterVector parse_parameters(
    const nlohmann::json& input_json)
{
    if (!input_json.contains("parameters") ||
        !input_json["parameters"].is_array() ||
        input_json["parameters"].size() != 15) {
        throw std::runtime_error(
            "Invalid input JSON: 'parameters' must be an array of 15 doubles.");
    }

    SunPositionAlgorithm::ParameterVector parameters{};
    for (std::size_t i = 0; i < parameters.size(); ++i) {
        parameters[i] = input_json["parameters"][i].get<double>();
    }

    return parameters;
}

GeoLocation parse_location(const nlohmann::json& input_json)
{
    if (!input_json.contains("location") || !input_json["location"].is_object()) {
        throw std::runtime_error(
            "Invalid input JSON: 'location' must be an object.");
    }

    const nlohmann::json& location_json = input_json["location"];
    if (!location_json.contains("longitude") ||
        !location_json.contains("latitude")) {
        throw std::runtime_error(
            "Invalid input JSON: 'location' must contain 'longitude' and 'latitude'.");
    }

    GeoLocation location{};
    location.longitude = location_json["longitude"].get<double>();
    location.latitude = location_json["latitude"].get<double>();
    return location;
}

std::string parse_mica_binary_file_path(const nlohmann::json& input_json)
{
    if (!input_json.contains("mica_binary_file_path") ||
        !input_json["mica_binary_file_path"].is_string()) {
        throw std::runtime_error(
            "Invalid input JSON: 'mica_binary_file_path' must be a string.");
    }

    return input_json["mica_binary_file_path"].get<std::string>();
}

nlohmann::json make_output_json(const EvaluationMetrics& metrics)
{
    nlohmann::json output_json;
    output_json["mean_angular_error"] = metrics.mean_angular_error;
    output_json["mean_azimuth_error"] = metrics.mean_azimuth_error;
    output_json["mean_zenith_error"] = metrics.mean_zenith_error;
    output_json["max_angular_error"] = metrics.max_angular_error;
    output_json["max_azimuth_error"] = metrics.max_azimuth_error;
    output_json["max_zenith_error"] = metrics.max_zenith_error;
    output_json["sample_count"] = metrics.sample_count;
    return output_json;
}

} // namespace

int main(int argc, char* argv[])
{
    try {
        if (argc != 5 ||
            std::string(argv[1]) != "--input" ||
            std::string(argv[3]) != "--output") {
            std::cerr
                << "Usage: sun_position_evaluator --input input.json --output output.json\n";
            return 1;
        }

        const std::string input_json_path = argv[2];
        const std::string output_json_path = argv[4];

        std::ifstream input_file(input_json_path);
        if (!input_file.is_open()) {
            throw std::runtime_error(
                "Failed to open input JSON file: " + input_json_path);
        }

        nlohmann::json input_json;
        input_file >> input_json;

        const SunPositionAlgorithm::ParameterVector parameters =
            parse_parameters(input_json);
        const std::string mica_binary_file_path =
            parse_mica_binary_file_path(input_json);
        const GeoLocation location = parse_location(input_json);

        const SunPositionEvaluator evaluator(mica_binary_file_path, location);
        const EvaluationMetrics metrics = evaluator.evaluate(parameters);

        std::ofstream output_file(output_json_path);
        if (!output_file.is_open()) {
            throw std::runtime_error(
                "Failed to open output JSON file: " + output_json_path);
        }

        output_file << make_output_json(metrics).dump(4) << '\n';
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Error: " << exception.what() << '\n';
        return 1;
    }
}
