#include "basic_test_fixture.h"
#include "sun_position_evaluator.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

struct BenchmarkArguments {
    std::filesystem::path m_fixture_directory = "basic_test";
    int m_repetitions = 5;
};

BenchmarkArguments parse_command_line_arguments(int argc, char* argv[])
{
    BenchmarkArguments arguments{};

    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];

        if (argument == "--fixture-dir") {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing value after --fixture-dir.");
            }

            arguments.m_fixture_directory = argv[++i];
        }
        else if (argument == "--repetitions") {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing value after --repetitions.");
            }

            arguments.m_repetitions = std::stoi(argv[++i]);
        }
        else {
            throw std::runtime_error("Unknown command-line argument: " + argument);
        }
    }

    if (arguments.m_repetitions <= 0) {
        throw std::runtime_error("'--repetitions' must be a positive integer.");
    }

    return arguments;
}

double duration_to_milliseconds(
    const std::chrono::steady_clock::duration& duration)
{
    return std::chrono::duration<double, std::milli>(duration).count();
}

} // namespace

int main(int argc, char* argv[])
{
    try {
        const BenchmarkArguments arguments =
            parse_command_line_arguments(argc, argv);

        const BasicTestFixture fixture =
            load_basic_test_fixture(arguments.m_fixture_directory);
        ensure_basic_test_binary_exists(fixture);

        SunPositionEvaluator evaluator(
            fixture.m_binary_file_path,
            fixture.m_location);

        // One warm-up evaluation avoids counting one-time effects such as file
        // system cache misses or cold instruction cache behavior in the baseline.
        const EvaluationMetrics warmup_metrics =
            evaluator.evaluate(fixture.m_parameters);

        std::chrono::steady_clock::duration total_duration{};
        double best_duration_ms = std::numeric_limits<double>::infinity();

        for (int repetition = 0; repetition < arguments.m_repetitions; ++repetition) {
            const auto start_time = std::chrono::steady_clock::now();
            const EvaluationMetrics metrics =
                evaluator.evaluate(fixture.m_parameters);
            const auto end_time = std::chrono::steady_clock::now();

            const auto current_duration = end_time - start_time;
            total_duration += current_duration;
            best_duration_ms = std::min(
                best_duration_ms,
                duration_to_milliseconds(current_duration));

            if (metrics.sample_count != warmup_metrics.sample_count) {
                throw std::runtime_error(
                    "Benchmark run changed sample count unexpectedly.");
            }
        }

        const double total_duration_ms =
            duration_to_milliseconds(total_duration);
        const double average_duration_ms =
            total_duration_ms / static_cast<double>(arguments.m_repetitions);

        std::cout << "sun_position_benchmark\n";
        std::cout << "fixture_directory: " << fixture.m_fixture_directory.string() << '\n';
        std::cout << "sample_count: " << warmup_metrics.sample_count << '\n';
        std::cout << "objective_arcsec: "
                  << warmup_metrics.sun_vector_error_arcsec.average << '\n';
        std::cout << "openmp_enabled: "
                  << (sun_position_openmp_enabled() ? "true" : "false") << '\n';
        std::cout << "repetitions: " << arguments.m_repetitions << '\n';
        std::cout << "warmup_runs: 1\n";
        std::cout << "total_wall_time_ms: " << total_duration_ms << '\n';
        std::cout << "average_time_per_evaluation_ms: " << average_duration_ms << '\n';
        std::cout << "best_time_ms: " << best_duration_ms << '\n';
        return 0;
    }
    catch (const std::exception& exception) {
        std::cerr << "Benchmark failed: " << exception.what() << '\n';
        return 1;
    }
}
