#include "basic_test_fixture.h"
#include "sun_position_evaluator.h"

#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct MetricExpectation {
    const char* m_name = "";
    double m_expected_value = 0.0;
    double m_tolerance = 0.0;
};

std::filesystem::path parse_fixture_directory(int argc, char* argv[])
{
    std::filesystem::path fixture_directory = "basic_test";

    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--fixture-dir") {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing value after --fixture-dir.");
            }

            fixture_directory = argv[++i];
            continue;
        }

        throw std::runtime_error("Unknown command-line argument: " + argument);
    }

    return fixture_directory;
}

void verify_metric(
    double actual_value,
    const MetricExpectation& expectation)
{
    const double absolute_error =
        std::fabs(actual_value - expectation.m_expected_value);

    if (absolute_error > expectation.m_tolerance) {
        throw std::runtime_error(
            std::string("Metric '") + expectation.m_name +
            "' was " + std::to_string(actual_value) +
            ", expected " + std::to_string(expectation.m_expected_value) +
            " +/- " + std::to_string(expectation.m_tolerance) + ".");
    }
}

void verify_sample_count(
    std::size_t actual_sample_count,
    std::uint64_t expected_sample_count)
{
    if (actual_sample_count != expected_sample_count) {
        throw std::runtime_error(
            "Sample count mismatch. Actual: " +
            std::to_string(actual_sample_count) +
            ", expected: " + std::to_string(expected_sample_count) + ".");
    }
}

} // namespace

int main(int argc, char* argv[])
{
    try {
        const std::filesystem::path fixture_directory =
            parse_fixture_directory(argc, argv);

        const BasicTestFixture fixture =
            load_basic_test_fixture(fixture_directory);
        ensure_basic_test_binary_exists(fixture);

        SunPositionEvaluator evaluator(
            fixture.m_binary_file_path,
            fixture.m_location);

        const EvaluationMetrics metrics =
            evaluator.evaluate(fixture.m_parameters);

        verify_sample_count(
            metrics.sample_count,
            fixture.m_expected_sample_count);

        // These tolerances are intentionally small enough to catch scientific
        // regressions, while still allowing the paper's rounded Table 2 values
        // and tiny platform-specific libm differences to coexist.
        // Averages use +/- 0.05 arcsec because the published values are rounded
        // to 0.01 arcsec and the current implementation stays much closer.
        // Standard deviation and mean deviation use +/- 0.10 arcsec because the
        // paper reports only two decimals and trigonometric implementations can
        // differ slightly across Windows, Linux, and macOS.
        const MetricExpectation expectations[] = {
            {"azimuth.average", 0.18, 0.05},
            {"azimuth.standard_deviation", 11.80, 0.10},
            {"azimuth.mean_deviation", 8.38, 0.10},
            {"zenith.average", 0.05, 0.05},
            {"zenith.standard_deviation", 6.96, 0.10},
            {"zenith.mean_deviation", 5.40, 0.10},
            {"sun_vector.average", 8.78, 0.10},
            {"sun_vector.standard_deviation", 5.40, 0.10},
            {"sun_vector.mean_deviation", 4.36, 0.10}
        };

        verify_metric(metrics.azimuth_error_arcsec.average, expectations[0]);
        verify_metric(metrics.azimuth_error_arcsec.standard_deviation, expectations[1]);
        verify_metric(metrics.azimuth_error_arcsec.mean_deviation, expectations[2]);
        verify_metric(metrics.zenith_error_arcsec.average, expectations[3]);
        verify_metric(metrics.zenith_error_arcsec.standard_deviation, expectations[4]);
        verify_metric(metrics.zenith_error_arcsec.mean_deviation, expectations[5]);
        verify_metric(metrics.sun_vector_error_arcsec.average, expectations[6]);
        verify_metric(metrics.sun_vector_error_arcsec.standard_deviation, expectations[7]);
        verify_metric(metrics.sun_vector_error_arcsec.mean_deviation, expectations[8]);

        std::cout
            << "Regression baseline matched for " << metrics.sample_count
            << " samples.\n";
        return 0;
    }
    catch (const std::exception& exception) {
        std::cerr << "Regression test failed: " << exception.what() << '\n';
        return 1;
    }
}
