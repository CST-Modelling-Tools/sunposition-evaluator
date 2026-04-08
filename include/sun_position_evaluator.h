#ifndef SUN_POSITION_EVALUATOR_H
#define SUN_POSITION_EVALUATOR_H

#include "mica_binary_dataset.h"
#include "sun_position_algorithm.h"

#include <cstddef>
#include <string>
#include <vector>

struct ReferenceSample {
    TimePoint time_point;
    double reference_zenith;
    double reference_azimuth;
};

struct ErrorStatistics {
    double average = 0.0;
    double standard_deviation = 0.0;
    double mean_deviation = 0.0;
    double minimum = 0.0;
    double maximum = 0.0;
};

struct EvaluationMetrics {
    ErrorStatistics azimuth_error_arcsec;
    ErrorStatistics zenith_error_arcsec;
    ErrorStatistics sun_vector_error_arcsec;
    std::size_t sample_count = 0;
};

class SunPositionEvaluator {
public:
    SunPositionEvaluator(
        const std::string& mica_binary_file_path,
        const GeoLocation& location);

    const std::vector<ReferenceSample>& reference_samples() const noexcept;
    const GeoLocation& location() const noexcept;

    EvaluationMetrics evaluate(
        const SunPositionAlgorithm::ParameterVector& parameters) const;

private:
    std::vector<ReferenceSample> m_reference_samples;
    GeoLocation m_location;

    void load_reference_samples(const std::string& mica_binary_file_path);

    static double compute_signed_azimuth_error(
        double computed_azimuth,
        double reference_azimuth);

    static double compute_signed_zenith_error(
        double computed_zenith,
        double reference_zenith);

    static double compute_angular_error(
        double computed_azimuth,
        double computed_zenith,
        double reference_azimuth,
        double reference_zenith);

    static ErrorStatistics compute_statistics(
        const std::vector<double>& values);
};

#endif // SUN_POSITION_EVALUATOR_H