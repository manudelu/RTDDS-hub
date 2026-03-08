#pragma once
#include <cstdint>
#include <cstddef>
#include "HeaderMsg.hpp"

struct rt_orientation
{
    double x;
    double y;
    double z;
    double w;
};

struct rt_vector3
{
    double x;
    double y;
    double z;
};

struct rt_imu_msg
{
    rt_header header;

    rt_orientation orientation;
    //double orientation_covariance[9];

    rt_vector3 angular_velocity;
    //double angular_velocity_covariance[9];

    rt_vector3 linear_acceleration;
    //double linear_acceleration_covariance[9];         
};