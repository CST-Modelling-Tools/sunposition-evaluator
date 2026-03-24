#include "sun_position_algorithm.h"

#include <cmath>

SunPositionAlgorithm::SunPositionAlgorithm()
    : m_parameters(default_parameters())
{
}

SunPositionAlgorithm::SunPositionAlgorithm(const ParameterVector& parameters)
    : m_parameters(parameters)
{
}

const SunPositionAlgorithm::ParameterVector&
SunPositionAlgorithm::parameters() const noexcept
{
    return m_parameters;
}

SunPositionAlgorithm::ParameterVector
SunPositionAlgorithm::default_parameters() noexcept
{
    return ParameterVector{
        2.267127827,
        -0.00093003392670,
        4.895036035,
        0.01720279602,
        6.239468336,
        0.01720200135,
        0.03338320972,
        0.0003497596876,
        -0.0001544353226,
        -0.00000868972936,
        0.4090904909,
        -6.213605399e-9,
        0.00004418094944,
        6.697096103,
        0.06570984737
    };
}

SunCoordinates SunPositionAlgorithm::operator()(
    const TimePoint& time_point,
    const GeoLocation& location) const
{
    double elapsed_julian_days = 0.0;
    double decimal_hours = 0.0;

    {
        double julian_date = 0.0;
        long int aux1 = 0;
        long int aux2 = 0;

        decimal_hours =
            time_point.hours +
            (time_point.minutes + time_point.seconds / 60.0) / 60.0;

        aux1 = (time_point.month - 14) / 12;
        aux2 = (1461 * (time_point.year + 4800 + aux1)) / 4
            + (367 * (time_point.month - 2 - 12 * aux1)) / 12
            - (3 * ((time_point.year + 4900 + aux1) / 100)) / 4
            + time_point.day - 32075;

        julian_date =
            static_cast<double>(aux2) - 0.5 + decimal_hours / 24.0;

        elapsed_julian_days = julian_date - 2451545.0;
    }

    double ecliptic_longitude = 0.0;
    double ecliptic_obliquity = 0.0;

    {
        double mean_longitude = 0.0;
        double mean_anomaly = 0.0;
        double omega = 0.0;

        omega = m_parameters[0] + m_parameters[1] * elapsed_julian_days;
        mean_longitude = m_parameters[2] + m_parameters[3] * elapsed_julian_days;
        mean_anomaly = m_parameters[4] + m_parameters[5] * elapsed_julian_days;

        ecliptic_longitude =
            mean_longitude
            + m_parameters[6] * std::sin(mean_anomaly)
            + m_parameters[7] * std::sin(2.0 * mean_anomaly)
            + m_parameters[8]
            + m_parameters[9] * std::sin(omega);

        ecliptic_obliquity =
            m_parameters[10]
            + m_parameters[11] * elapsed_julian_days
            + m_parameters[12] * std::cos(omega);
    }

    double declination = 0.0;
    double right_ascension = 0.0;

    {
        const double sin_ecliptic_longitude = std::sin(ecliptic_longitude);
        const double y =
            std::cos(ecliptic_obliquity) * sin_ecliptic_longitude;
        const double x = std::cos(ecliptic_longitude);

        right_ascension = std::atan2(y, x);
        if (right_ascension < 0.0) {
            right_ascension += kTwoPi;
        }

        declination =
            std::asin(std::sin(ecliptic_obliquity) * sin_ecliptic_longitude);
    }

    SunCoordinates coordinates{};
    coordinates.declination = declination;

    {
        const double greenwich_mean_sidereal_time =
            m_parameters[13]
            + m_parameters[14] * elapsed_julian_days
            + decimal_hours;

        const double local_mean_sidereal_time =
            (greenwich_mean_sidereal_time * 15.0 + location.longitude) *
            kRadiansPerDegree;

        const double hour_angle = local_mean_sidereal_time - right_ascension;
        const double latitude_in_radians = location.latitude * kRadiansPerDegree;

        const double cos_latitude = std::cos(latitude_in_radians);
        const double sin_latitude = std::sin(latitude_in_radians);
        const double sin_hour_angle = std::sin(hour_angle);
        const double cos_hour_angle = std::cos(hour_angle);

        coordinates.bounded_hour_angle =
            std::atan2(sin_hour_angle, cos_hour_angle);

        double zenith_angle = std::acos(
            cos_latitude * cos_hour_angle * std::cos(declination)
            + std::sin(declination) * sin_latitude);

        const double y = -std::sin(hour_angle);
        const double x =
            std::tan(declination) * cos_latitude
            - sin_latitude * cos_hour_angle;

        double azimuth = std::atan2(y, x);
        if (azimuth < 0.0) {
            azimuth += kTwoPi;
        }

        const double parallax =
            (kEarthMeanRadiusKm / kAstronomicalUnitKm) *
            std::sin(zenith_angle);

        zenith_angle += parallax;

        coordinates.azimuth = azimuth;
        coordinates.zenith_angle = zenith_angle;
    }

    return coordinates;
}
