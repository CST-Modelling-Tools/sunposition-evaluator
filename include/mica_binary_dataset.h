#ifndef MICA_BINARY_DATASET_H
#define MICA_BINARY_DATASET_H

#include <array>
#include <cstdint>
#include <type_traits>

struct MicaBinaryDatasetHeader {
    std::array<char, 8> magic;
    std::uint64_t version;
    std::uint64_t sample_count;
};

struct MicaBinaryReferenceSample {
    std::int32_t year;
    std::int32_t month;
    std::int32_t day;
    double hours;
    double minutes;
    double seconds;
    double zenith_angle;
    double azimuth;
};

static_assert(std::is_standard_layout_v<MicaBinaryDatasetHeader>);
static_assert(std::is_standard_layout_v<MicaBinaryReferenceSample>);
static_assert(sizeof(MicaBinaryDatasetHeader) == 24);
static_assert(sizeof(MicaBinaryReferenceSample) == 56);

#endif // MICA_BINARY_DATASET_H
