#pragma once
#include <cstdint>
#include <cstddef>
#include "HeaderMsg.hpp"

template <size_t NUM_DOFS>
struct rt_joint_state_msg
{
    rt_header header;
    double   positions[NUM_DOFS];   // Joint positions  [rad]
    double   velocities[NUM_DOFS];  // Joint velocities [rad/s]
    double   efforts[NUM_DOFS];     // Joint efforts    [Nm]
};

using SpotJointStateMsg = rt_joint_state_msg<12>;
using UR5JointStateMsg  = rt_joint_state_msg<6>;
