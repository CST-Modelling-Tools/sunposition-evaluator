#ifndef SUN_POSITION_ALGORITHM_H
#define SUN_POSITION_ALGORITHM_H

#include <array>
#include <numbers>

// Physical and mathematical constants
constexpr double pi = std::numbers::pi;
constexpr double twopi = 2.0 * pi;
constexpr double rad = pi / 180.0;
constexpr double dEarthMeanRadius = 6371.01;        // km
constexpr double dAstronomicalUnit = 149597870.7;   // km

// Universal time specification
struct TimePoint {
    int year;
    int month;
    int day;
    double hours;
    double minutes;
    double seconds;
};

// Geographic position
struct GeoLocation {
    double longitude;   // degrees
    double latitude;    // degrees
};

// Output solar coordinates
struct SunCoordinates {
    double zenith_angle;        // rad
    double azimuth;             // rad, from North to East
    double declination;         // rad
    double bounded_hour_angle;  // rad, in [-pi, pi]
};

class SunPositionAlgorithm 
{
public:
    using ParameterVector = std::array<double, 15>;

    SunPositionAlgorithm();
    explicit SunPositionAlgorithm(const ParameterVector& parameters);

    void operator()(
        const TimePoint& time_point,
        const GeoLocation& location,
        SunCoordinates* sun_coordinates) const;

    const ParameterVector& parameters() const noexcept;

    static ParameterVector default_parameters() noexcept;

    private:
    ParameterVector m_parameters;
};

#endif // SUN_POSITION_ALGORITHM_H