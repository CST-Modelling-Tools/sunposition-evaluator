#ifndef SUN_POSITION_ALGORITHM_H
#define SUN_POSITION_ALGORITHM_H

#include <array>
#include <numbers>

constexpr double kPi = std::numbers::pi;
constexpr double kTwoPi = 2.0 * kPi;
constexpr double kRadiansPerDegree = kPi / 180.0;
constexpr double kEarthMeanRadiusKm = 6371.01;
constexpr double kAstronomicalUnitKm = 149597870.7;

struct TimePoint {
    int year;
    int month;
    int day;
    double hours;
    double minutes;
    double seconds;
};

struct GeoLocation {
    double longitude;
    double latitude;
};

struct SunCoordinates {
    double zenith_angle;
    double azimuth;
    double declination;
    double bounded_hour_angle;
};

class SunPositionAlgorithm {
public:
    using ParameterVector = std::array<double, 15>;

    SunPositionAlgorithm();
    explicit SunPositionAlgorithm(const ParameterVector& parameters);

    SunCoordinates operator()(
        const TimePoint& time_point,
        const GeoLocation& location) const;

    const ParameterVector& parameters() const noexcept;

    static ParameterVector default_parameters() noexcept;

private:
    ParameterVector m_parameters;
};

#endif // SUN_POSITION_ALGORITHM_H
