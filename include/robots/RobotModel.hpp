#pragma once
#include <cstddef>

template <size_t MAX_DOFS>
struct RobotModel
{
    static constexpr size_t dofs = MAX_DOFS;

    const char* joint_names[MAX_DOFS]; // ROS joint names, used for DDS topic
    const char* pipe_names[MAX_DOFS];  // XDDP pipe labels 
    size_t num_pipes;                  // how many pipe_names are valid

    double joint_min[MAX_DOFS];
    double joint_max[MAX_DOFS];
    double home_position[MAX_DOFS];

    const char* imu_pipe_name;
};

// Spot — 12-DOF quadruped
using SpotRobot = RobotModel<12>;
constexpr SpotRobot spot {
    .joint_names = {
        "front_left_hip_x",  "front_left_hip_y",  "front_left_knee",
        "front_right_hip_x", "front_right_hip_y", "front_right_knee",
        "rear_left_hip_x",   "rear_left_hip_y",   "rear_left_knee",
        "rear_right_hip_x",  "rear_right_hip_y",  "rear_right_knee"
    },
    .pipe_names  = {
        "NoNe@Nov_Motor_id_4",  "NoNe@Synap_Motor_id_3", "NoNe@Synap_Motor_id_2"
    },
    .num_pipes = 3,
    .joint_min = {
        -0.785, -0.899, -2.793,
        -0.785, -0.899, -2.793,
        -0.785, -0.899, -2.793,
        -0.785, -0.899, -2.793
    },
    .joint_max = {
        0.785, 2.295, -0.255,
        0.785, 2.295, -0.248,
        0.785, 2.295, -0.267,
        0.785, 2.295, -0.258
    },
    .home_position = {
        0.0, 0.0, -1.5238505,
        0.0, 0.0, -1.5202315,
        0.0, 0.0, -1.5300265,
        0.0, 0.0, -1.5253125
    },
    // If no IMU data is available, set this to nullptr or an empty string
    .imu_pipe_name = { nullptr }, 
};

// UR5 — 6-DOF manipulator
using UR5Robot = RobotModel<6>;
constexpr UR5Robot ur5 {
    .joint_names = {
        "shoulder_pan_joint",  "shoulder_lift_joint", "elbow_joint",
        "wrist_1_joint",       "wrist_2_joint",       "wrist_3_joint"
    },
    .pipe_names = {
        "NoNe@Synap_Motor_id_4",  "NoNe@Synap_Motor_id_3", "NoNe@Nov_Motor_id_5"
    },
    .num_pipes = 3,
    .joint_min = {
        -6.133, -6.133, -2.992,
        -6.133, -6.133, -6.133
    },
    .joint_max = {
        6.133, 6.133, 2.992,
        6.133, 6.133, 6.133
    },
    .home_position = {
        0.0, 0.0, 0.0,
        0.0, 0.0, 0.0
    },
    // If no IMU data is available, set this to nullptr or an empty string
    .imu_pipe_name = { nullptr }, 
};
