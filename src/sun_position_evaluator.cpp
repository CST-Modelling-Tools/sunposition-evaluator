#include "sun_position_evaluator.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {

double clamp_to_unit_interval(double value)
{
    return std::max(-1.0, std::min(1.0, value));
}

} // namespace

SunPositionEvaluator::SunPositionEvaluator(
    const std::string& mica_binary_file_path,
    const GeoLocation& location)
    : m_location(location)
{
    load_reference_samples_from_binary(mica_binary_file_path);
}

const std::vector<ReferenceSample>&
SunPositionEvaluator::reference_samples() const noexcept
{
    return m_reference_samples;
}

const GeoLocation&
SunPositionEvaluator::location() const noexcept
{
    return m_location;
}

EvaluationMetrics SunPositionEvaluator::evaluate(
    const SunPositionAlgorithm::ParameterVector& parameters) const
{
    SunPositionAlgorithm algorithm(parameters);

    double sum_angular_error = 0.0;
    double sum_azimuth_error = 0.0;
    double sum_zenith_error = 0.0;

    double max_angular_error = 0.0;
    double max_azimuth_error = 0.0;
    double max_zenith_error = 0.0;

    for (const auto& sample : m_reference_samples) {
        SunCoordinates computed_coordinates{};
        algorithm(sample.time_point, m_location, &computed_coordinates);

        const double azimuth_error = compute_azimuth_error(
            computed_coordinates.azimuth,
            sample.reference_azimuth);

        const double zenith_error = compute_zenith_error(
            computed_coordinates.zenith_angle,
            sample.reference_zenith_angle);

        const double angular_error = compute_angular_error(
            computed_coordinates.azimuth,
            computed_coordinates.zenith_angle,
            sample.reference_azimuth,
            sample.reference_zenith_angle);

        sum_angular_error += angular_error;
        sum_azimuth_error += azimuth_error;
        sum_zenith_error += zenith_error;

        max_angular_error = std::max(max_angular_error, angular_error);
        max_azimuth_error = std::max(max_azimuth_error, azimuth_error);
        max_zenith_error = std::max(max_zenith_error, zenith_error);
    }

    const double sample_count = static_cast<double>(m_reference_samples.size());

    EvaluationMetrics metrics{};
    metrics.mean_angular_error = sum_angular_error / sample_count;
    metrics.mean_azimuth_error = sum_azimuth_error / sample_count;
    metrics.mean_zenith_error = sum_zenith_error / sample_count;
    metrics.max_angular_error = max_angular_error;
    metrics.max_azimuth_error = max_azimuth_error;
    metrics.max_zenith_error = max_zenith_error;
    metrics.sample_count = m_reference_samples.size();

    return metrics;
}

void SunPositionEvaluator::load_reference_samples_from_binary(
    const std::string& mica_binary_file_path)
{
    std::ifstream file(mica_binary_file_path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(
            "Failed to open MICA binary file: " + mica_binary_file_path);
    }

    MicaBinaryDatasetHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file) {
        throw std::runtime_error(
            "Failed to read binary dataset header from file: " +
            mica_binary_file_path);
    }

    if (std::memcmp(header.magic, "MICABIN1", 8) != 0) {
        throw std::runtime_error(
            "Invalid binary dataset magic in file: " + mica_binary_file_path);
    }

    if (header.version != 1) {
        throw std::runtime_error(
            "Unsupported binary dataset version in file: " +
            mica_binary_file_path);
    }

    m_reference_samples.clear();
    m_reference_samples.reserve(static_cast<std::size_t>(header.sample_count));

    MicaBinaryReferenceSample binary_sample{};
    for (std::uint64_t i = 0; i < header.sample_count; ++i) {
        file.read(
            reinterpret_cast<char*>(&binary_sample),
            sizeof(binary_sample));

        if (!file) {
            throw std::runtime_error(
                "Failed to read binary sample from file: " +
                mica_binary_file_path);
        }

        ReferenceSample sample{};
        sample.time_point.year = binary_sample.year;
        sample.time_point.month = binary_sample.month;
        sample.time_point.day = binary_sample.day;
        sample.time_point.hours = binary_sample.hours;
        sample.time_point.minutes = binary_sample.minutes;
        sample.time_point.seconds = binary_sample.seconds;
        sample.reference_zenith_angle = binary_sample.zenith_angle;
        sample.reference_azimuth = binary_sample.azimuth;

        m_reference_samples.push_back(sample);
    }

    if (m_reference_samples.empty()) {
        throw std::runtime_error(
            "No reference samples were loaded from binary file: " +
            mica_binary_file_path);
    }
}

double SunPositionEvaluator::compute_azimuth_error(
    double computed_azimuth,
    double reference_azimuth)
{
    double error = std::fabs(computed_azimuth - reference_azimuth);
    if (error > pi) {
        error = twopi - error;
    }
    return error;
}

double SunPositionEvaluator::compute_zenith_error(
    double computed_zenith_angle,
    double reference_zenith_angle)
{
    return std::fabs(computed_zenith_angle - reference_zenith_angle);
}

double SunPositionEvaluator::compute_angular_error(
    double computed_azimuth,
    double computed_zenith_angle,
    double reference_azimuth,
    double reference_zenith_angle)
{
    const double computed_x =
        std::sin(computed_zenith_angle) * std::sin(computed_azimuth);
    const double computed_y =
        std::sin(computed_zenith_angle) * std::cos(computed_azimuth);
    const double computed_z =
        std::cos(computed_zenith_angle);

    const double reference_x =
        std::sin(reference_zenith_angle) * std::sin(reference_azimuth);
    const double reference_y =
        std::sin(reference_zenith_angle) * std::cos(reference_azimuth);
    const double reference_z =
        std::cos(reference_zenith_angle);

    const double dot_product =
        computed_x * reference_x +
        computed_y * reference_y +
        computed_z * reference_z;

    return std::acos(clamp_to_unit_interval(dot_product));
}