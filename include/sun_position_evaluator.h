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

struct EvaluationMetrics {
    double mean_angular_error;
    double mean_azimuth_error;
    double mean_zenith_error;
    double max_angular_error;
    double max_azimuth_error;
    double max_zenith_error;
    std::size_t sample_count;
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
    void load_reference_samples(const std::string& mica_binary_file_path);

    static double compute_azimuth_error(
        double computed_azimuth,
        double reference_azimuth);

    static double compute_zenith_error(
        double computed_zenith,
        double reference_zenith);

    static double compute_angular_error(
        double computed_azimuth,
        double computed_zenith,
        double reference_azimuth,
        double reference_zenith);

    std::vector<ReferenceSample> m_reference_samples;
    GeoLocation m_location;
};

#endif // SUN_POSITION_EVALUATOR_H
