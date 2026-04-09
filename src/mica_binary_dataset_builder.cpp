#include "mica_binary_dataset_builder.h"

#include "mica_binary_dataset.h"

#include <cstring>
#include <fstream>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::vector<std::string> split_csv_line(const std::string& line)
{
    std::vector<std::string> tokens;
    std::stringstream line_stream(line);
    std::string token;

    while (std::getline(line_stream, token, ',')) {
        tokens.push_back(token);
    }

    return tokens;
}

double degrees_to_radians(const std::string& value)
{
    const long double degrees = std::stold(value);
    const long double radians =
        degrees * std::numbers::pi_v<long double> / 180.0L;
    return static_cast<double>(radians);
}

MicaBinaryReferenceSample parse_csv_row(
    const std::string& line,
    std::size_t line_number)
{
    const std::vector<std::string> fields = split_csv_line(line);
    if (fields.size() != 8) {
        throw std::runtime_error(
            "Invalid CSV row at line " + std::to_string(line_number) +
            ": expected 8 comma-separated values.");
    }

    MicaBinaryReferenceSample sample{};
    sample.year = std::stoi(fields[0]);
    sample.month = std::stoi(fields[1]);
    sample.day = std::stoi(fields[2]);
    sample.hours = std::stod(fields[3]);
    sample.minutes = std::stod(fields[4]);
    sample.seconds = std::stod(fields[5]);
    sample.zenith_angle = degrees_to_radians(fields[6]);
    sample.azimuth = degrees_to_radians(fields[7]);
    return sample;
}

} // namespace

void build_mica_binary_dataset(
    const std::filesystem::path& input_csv_path,
    const std::filesystem::path& output_binary_path)
{
    std::ifstream input_file(input_csv_path);
    if (!input_file.is_open()) {
        throw std::runtime_error(
            "Failed to open input CSV file: " + input_csv_path.string());
    }

    std::vector<MicaBinaryReferenceSample> samples;
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input_file, line)) {
        ++line_number;
        if (line.empty()) {
            continue;
        }

        samples.push_back(parse_csv_row(line, line_number));
    }

    const std::filesystem::path output_directory =
        output_binary_path.parent_path();
    if (!output_directory.empty()) {
        std::filesystem::create_directories(output_directory);
    }

    std::ofstream output_file(output_binary_path, std::ios::binary);
    if (!output_file.is_open()) {
        throw std::runtime_error(
            "Failed to open output binary file: " + output_binary_path.string());
    }

    MicaBinaryDatasetHeader header{};
    std::memcpy(header.magic.data(), "MICABIN1", 8);
    header.version = 1;
    header.sample_count = static_cast<std::uint64_t>(samples.size());

    output_file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    if (!samples.empty()) {
        output_file.write(
            reinterpret_cast<const char*>(samples.data()),
            static_cast<std::streamsize>(
                samples.size() * sizeof(MicaBinaryReferenceSample)));
    }

    if (!output_file) {
        throw std::runtime_error(
            "Failed while writing binary dataset: " + output_binary_path.string());
    }
}
