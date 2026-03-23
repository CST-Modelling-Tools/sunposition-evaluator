#ifndef MICA_BINARY_DATASET_H
#define MICA_BINARY_DATASET_H

#include <cstdint>

struct MicaBinaryDatasetHeader {
    char magic[8];
    std::uint64_t version;
    std::uint64_t sample_count;
};

struct MicaBinaryReferenceSample {
    int year;
    int month;
    int day;
    double hours;
    double minutes;
    double seconds;
    double zenith_angle;  // radians
    double azimuth;       // radians
};

#endif // MICA_BINARY_DATASET_H