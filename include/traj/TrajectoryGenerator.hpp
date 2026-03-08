#pragma once
#include <array>
#include <cstddef>
#include "robots/RobotModel.hpp"

template <typename Traj, size_t NUM_DOFS>
class TrajectoryGenerator
{
    static_assert(NUM_DOFS > 0, "NUM_DOFS must be positive");

public:
    TrajectoryGenerator(const RobotModel<NUM_DOFS>& robot,
                        const std::array<double, NUM_DOFS>& goal,
                        double duration)
        : duration_(duration)
    {
        for (size_t i = 0; i < NUM_DOFS; ++i)
        {
            const double q0 = robot.home_position[i];
            const double qf = clamp(goal[i], robot.joint_min[i], robot.joint_max[i]);
            trajectories_[i] = Traj(q0, qf, duration_);
        }
    }

    void compute(double t,
                 double (&pos)[NUM_DOFS],
                 double (&vel)[NUM_DOFS]) const
    {
        const double tc = clamp(t, 0.0, duration_);
        for (size_t i = 0; i < NUM_DOFS; ++i)
        {
            pos[i] = trajectories_[i].position(tc);
            vel[i] = trajectories_[i].velocity(tc);
        }
    }

private:
    static double clamp(double v, double lo, double hi)
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    double duration_;
    Traj   trajectories_[NUM_DOFS];
};
