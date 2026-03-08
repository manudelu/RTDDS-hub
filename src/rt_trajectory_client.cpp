#include "rt/rt_client.hpp"
#include "rt/SharedBuffer.hpp"
#include "robots/RobotModel.hpp"
#include "traj/TrajectoryGenerator.hpp"
#include "traj/TrajectoryTypes.hpp"
#include "msgs/JointStateMsg.hpp"

#include <time.h>
#include <array>
#include <iostream>
#include <cstring>

namespace {

    inline uint64_t timespec_to_ns(const struct timespec& ts) noexcept
    {
        return static_cast<uint64_t>(ts.tv_sec)  * 1'000'000'000ULL + static_cast<uint64_t>(ts.tv_nsec);
    }

    inline double timespec_diff_s(const struct timespec& a, const struct timespec& b) noexcept
    {
        return static_cast<double>(a.tv_sec  - b.tv_sec) + static_cast<double>(a.tv_nsec - b.tv_nsec) * 1e-9;
    }

    template <typename RobotT, typename MsgT,
              boost::lockfree::spsc_queue<MsgT, boost::lockfree::capacity<RING_CAPACITY>>& Ring>
    void run_rt_loop(const RobotT& robot,
                     const std::array<double, RobotT::dofs>& goal,
                     long period_ns)
    {
        constexpr size_t DOFS = RobotT::dofs;

        TrajectoryGenerator<Quintic, DOFS> traj_gen(robot, goal, 5.0);

        MsgT msg{};
        uint64_t seq = 0;
        double pos[DOFS];
        double vel[DOFS];

        struct timespec t0, ts_now;
        clock_gettime(CLOCK_MONOTONIC, &t0);

        const struct timespec sleep_ts = {
            period_ns / 1'000'000'000L,
            period_ns % 1'000'000'000L
        };

        std::cout << "[rt_client] " << robot.joint_names[0]
                << " (..." << DOFS << " DOF) loop started, period "
                << period_ns / 1'000'000 << " ms\n";

        while (g_running.load(std::memory_order_relaxed))
        {
            clock_gettime(CLOCK_MONOTONIC, &ts_now);

            const double t = timespec_diff_s(ts_now, t0);

            msg.header.seq = seq++;
            msg.header.timestamp_ns = timespec_to_ns(ts_now);

            traj_gen.compute(t, pos, vel);

            for (size_t i = 0; i < DOFS; ++i)
            {
                msg.positions[i] = pos[i];
                msg.velocities[i] = vel[i];
                msg.efforts[i] = 0.0;  
            }

            Ring.push(msg);

            clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep_ts, nullptr);
        }

        std::cout << "[rt_client] loop exited.\n";
    }

}

void* client_spot(void* arg)
{
    static constexpr std::array<double, SpotRobot::dofs> goal = {{
        0.0,  0.4, -0.5,
        0.0,  0.5, -0.4,
        0.0,  0.4, -0.5,
        0.0,  0.5, -0.4
    }};

    constexpr long PERIOD_NS = 50'000'000L;   // 50 ms → 20 Hz
    run_rt_loop<SpotRobot, SpotJointStateMsg, g_spot_ring>(spot, goal, PERIOD_NS);
    return nullptr;
}

void* client_ur5(void* arg)
{
    static constexpr std::array<double, UR5Robot::dofs> goal = {{
         1.57, -1.57,  1.57,
        -1.57,  1.57,  0.0
    }};

    constexpr long PERIOD_NS = 50'000'000L;   // 50 ms → 20 Hz
    run_rt_loop<UR5Robot, UR5JointStateMsg, g_ur5_ring>(ur5, goal, PERIOD_NS);
    return nullptr;
}