#include "sun_position_evaluator.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace {

constexpr double RAD_TO_ARCSEC = 180.0 * 3600.0 / std::numbers::pi;

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
    load_reference_samples(mica_binary_file_path);
}

const std::vector<ReferenceSample>&
SunPositionEvaluator::reference_samples() const noexcept
{
    return m_reference_samples;
}

const GeoLocation& SunPositionEvaluator::location() const noexcept
{
    return m_location;
}

EvaluationMetrics SunPositionEvaluator::evaluate(
    const SunPositionAlgorithm::ParameterVector& parameters) const
{
    if (m_reference_samples.empty()) {
        throw std::runtime_error("Cannot evaluate an empty reference dataset.");
    }

    SunPositionAlgorithm algorithm(parameters);

    std::vector<double> azimuth_errors_arcsec;
    std::vector<double> zenith_errors_arcsec;
    std::vector<double> sun_vector_errors_arcsec;

    azimuth_errors_arcsec.reserve(m_reference_samples.size());
    zenith_errors_arcsec.reserve(m_reference_samples.size());
    sun_vector_errors_arcsec.reserve(m_reference_samples.size());

    for (const ReferenceSample& sample : m_reference_samples) {
        const SunCoordinates computed_coordinates =
            algorithm(sample.time_point, m_location);

        const double signed_azimuth_error_rad = compute_signed_azimuth_error(
            computed_coordinates.azimuth,
            sample.reference_azimuth);

        const double signed_zenith_error_rad = compute_signed_zenith_error(
            computed_coordinates.zenith_angle,
            sample.reference_zenith);

        const double angular_error_rad = compute_angular_error(
            computed_coordinates.azimuth,
            computed_coordinates.zenith_angle,
            sample.reference_azimuth,
            sample.reference_zenith);

        azimuth_errors_arcsec.push_back(
            signed_azimuth_error_rad * RAD_TO_ARCSEC);

        zenith_errors_arcsec.push_back(
            signed_zenith_error_rad * RAD_TO_ARCSEC);

        sun_vector_errors_arcsec.push_back(
            angular_error_rad * RAD_TO_ARCSEC);
    }

    EvaluationMetrics metrics{};
    metrics.azimuth_error_arcsec =
        compute_statistics(azimuth_errors_arcsec);
    metrics.zenith_error_arcsec =
        compute_statistics(zenith_errors_arcsec);
    metrics.sun_vector_error_arcsec =
        compute_statistics(sun_vector_errors_arcsec);
    metrics.sample_count = m_reference_samples.size();

    return metrics;
}

void SunPositionEvaluator::load_reference_samples(
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
            "Failed to read binary dataset header: " + mica_binary_file_path);
    }

    if (std::memcmp(header.magic.data(), "MICABIN1", 8) != 0) {
        throw std::runtime_error(
            "Invalid MICA binary dataset magic: " + mica_binary_file_path);
    }

    if (header.version != 1) {
        throw std::runtime_error(
            "Unsupported MICA binary dataset version: " +
            std::to_string(header.version));
    }

    m_reference_samples.clear();
    m_reference_samples.reserve(static_cast<std::size_t>(header.sample_count));

    for (std::uint64_t i = 0; i < header.sample_count; ++i) {
        MicaBinaryReferenceSample binary_sample{};
        file.read(reinterpret_cast<char*>(&binary_sample), sizeof(binary_sample));
        if (!file) {
            throw std::runtime_error(
                "Failed to read MICA binary reference sample.");
        }

        ReferenceSample sample{};
        sample.time_point.year = binary_sample.year;
        sample.time_point.month = binary_sample.month;
        sample.time_point.day = binary_sample.day;
        sample.time_point.hours = binary_sample.hours;
        sample.time_point.minutes = binary_sample.minutes;
        sample.time_point.seconds = binary_sample.seconds;
        sample.reference_zenith = binary_sample.zenith_angle;
        sample.reference_azimuth = binary_sample.azimuth;

        m_reference_samples.push_back(sample);
    }

    if (m_reference_samples.empty()) {
        throw std::runtime_error(
            "No reference samples were loaded from MICA binary dataset.");
    }
}

double SunPositionEvaluator::compute_signed_azimuth_error(
    double computed_azimuth,
    double reference_azimuth)
{
    double error = computed_azimuth - reference_azimuth;

    while (error <= -kPi) {
        error += kTwoPi;
    }

    while (error > kPi) {
        error -= kTwoPi;
    }

    return error;
}

double SunPositionEvaluator::compute_signed_zenith_error(
    double computed_zenith,
    double reference_zenith)
{
    return computed_zenith - reference_zenith;
}

double SunPositionEvaluator::compute_angular_error(
    double computed_azimuth,
    double computed_zenith,
    double reference_azimuth,
    double reference_zenith)
{
    const double computed_x =
        std::sin(computed_zenith) * std::sin(computed_azimuth);
    const double computed_y =
        std::sin(computed_zenith) * std::cos(computed_azimuth);
    const double computed_z = std::cos(computed_zenith);

    const double reference_x =
        std::sin(reference_zenith) * std::sin(reference_azimuth);
    const double reference_y =
        std::sin(reference_zenith) * std::cos(reference_azimuth);
    const double reference_z = std::cos(reference_zenith);

    const double dot_product =
        computed_x * reference_x +
        computed_y * reference_y +
        computed_z * reference_z;

    return std::acos(clamp_to_unit_interval(dot_product));
}

ErrorStatistics SunPositionEvaluator::compute_statistics(
    const std::vector<double>& values)
{
    if (values.empty()) {
        throw std::runtime_error("Cannot compute statistics for an empty vector.");
    }

    const double sample_count = static_cast<double>(values.size());

    double sum = 0.0;
    double minimum = std::numeric_limits<double>::infinity();
    double maximum = -std::numeric_limits<double>::infinity();

    for (double value : values) {
        sum += value;
        minimum = std::min(minimum, value);
        maximum = std::max(maximum, value);
    }

    const double average = sum / sample_count;

    double squared_sum = 0.0;
    double mean_deviation_sum = 0.0;

    for (double value : values) {
        const double delta = value - average;
        squared_sum += delta * delta;
        mean_deviation_sum += std::fabs(delta);
    }

    ErrorStatistics statistics{};
    statistics.average = average;
    statistics.standard_deviation =
        std::sqrt(squared_sum / sample_count);
    statistics.mean_deviation =
        mean_deviation_sum / sample_count;
    statistics.minimum = minimum;
    statistics.maximum = maximum;

    return statistics;
}