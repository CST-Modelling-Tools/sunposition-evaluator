#include "mica_binary_dataset.h"

#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::vector<std::string> split_csv_line(const std::string& line)
{
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;

    while (std::getline(ss, token, ',')) {
        tokens.push_back(token);
    }

    return tokens;
}

double degrees_to_radians_high_precision(const std::string& text)
{
    const long double degrees = std::stold(text);
    const long double radians =
        degrees * std::numbers::pi_v<long double> / 180.0L;
    return static_cast<double>(radians);
}

MicaBinaryReferenceSample parse_csv_row(const std::string& line)
{
    const auto fields = split_csv_line(line);
    if (fields.size() != 8) {
        throw std::runtime_error(
            "Invalid CSV row: expected exactly 8 comma-separated values.");
    }

    MicaBinaryReferenceSample sample{};

    sample.year = std::stoi(fields[0]);
    sample.month = std::stoi(fields[1]);
    sample.day = std::stoi(fields[2]);
    sample.hours = std::stod(fields[3]);
    sample.minutes = std::stod(fields[4]);
    sample.seconds = std::stod(fields[5]);
    sample.zenith_angle = degrees_to_radians_high_precision(fields[6]);
    sample.azimuth = degrees_to_radians_high_precision(fields[7]);

    return sample;
}

} // namespace

int main(int argc, char* argv[])
{
    try {
        if (argc != 3) {
            std::cerr
                << "Usage: build_mica_binary_dataset <input.csv> <output.bin>\n";
            return 1;
        }

        const std::string input_csv_path = argv[1];
        const std::string output_bin_path = argv[2];

        std::ifstream input_file(input_csv_path);
        if (!input_file.is_open()) {
            throw std::runtime_error(
                "Failed to open input CSV file: " + input_csv_path);
        }

        std::ofstream output_file(output_bin_path, std::ios::binary);
        if (!output_file.is_open()) {
            throw std::runtime_error(
                "Failed to open output binary file: " + output_bin_path);
        }

        MicaBinaryDatasetHeader header{};
        std::memcpy(header.magic, "MICABIN1", 8);
        header.version = 1;
        header.sample_count = 0;

        output_file.write(
            reinterpret_cast<const char*>(&header),
            sizeof(header));

        std::string line;
        std::uint64_t sample_count = 0;

        while (std::getline(input_file, line)) {
            if (line.empty()) {
                continue;
            }

            const MicaBinaryReferenceSample sample = parse_csv_row(line);

            output_file.write(
                reinterpret_cast<const char*>(&sample),
                sizeof(sample));

            ++sample_count;
        }

        if (!output_file) {
            throw std::runtime_error(
                "Error while writing binary dataset: " + output_bin_path);
        }

        header.sample_count = sample_count;

        output_file.seekp(0, std::ios::beg);
        output_file.write(
            reinterpret_cast<const char*>(&header),
            sizeof(header));

        output_file.close();

        std::cout << "Wrote " << sample_count
                  << " samples to " << output_bin_path << '\n';

        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}