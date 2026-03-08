#include "dds/JointStatePublisher.hpp"
#include "protobuf/pdo_utils.hpp"
#include "rt/SharedBuffer.hpp"
#include "rt/xddp_reader.hpp"
#include "robots/RobotModel.hpp"
#include "msgs/JointStateMsg.hpp"

#include <sys/epoll.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

static constexpr int EPOLL_TIMEOUT_MS = 5;

template <typename RobotT, typename MsgT>
void run_real_dds_loop(const RobotT& robot, int domain_id)
{
    constexpr size_t DOFS = RobotT::dofs;

    // Open XDDP pipes (missing ones become zero joints)
    XddpReader<DOFS> reader;
    reader.open(robot.pipe_names, /*optional=*/true);

    // Set up epoll — thread sleeps until one or more pipes have new data
    const int epfd = epoll_create1(0);
    if (epfd < 0)
    {
        std::cerr << "[JointStatePublisher] epoll_create1: " << strerror(errno) << '\n';
        g_running.store(false, std::memory_order_relaxed);
        return;
    }

    for (size_t i = 0; i < DOFS; ++i)
    {
        if (!reader.is_open(i))
            continue;

        struct epoll_event ev{};
        ev.events   = EPOLLIN;
        ev.data.u32 = static_cast<uint32_t>(i); // joint index stored for retrieval on wake

        if (epoll_ctl(epfd, EPOLL_CTL_ADD, reader.fd(i), &ev) < 0)
            std::cerr << "[JointStatePublisher] epoll_ctl ADD joint " << i
                      << ": " << strerror(errno) << '\n';
    }

    // Initialise DDS publisher
    const std::vector<std::string> joint_names(robot.joint_names, robot.joint_names + DOFS);

    JointStatePublisher<DOFS> pub;
    if (!pub.init(joint_names, domain_id))
    {
        std::cerr << "[JointStatePublisher] failed to initialise publisher\n";
        g_running.store(false, std::memory_order_relaxed);
        ::close(epfd);
        return;
    }

    std::cout << "[JointStatePublisher] Loop started ("
              << DOFS << " DOF, domain " << domain_id << ")\n";

    MsgT msg{};
    uint64_t seq = 0;

    alignas(8) uint8_t raw[512];
    struct epoll_event events[DOFS];

    while (g_running.load(std::memory_order_relaxed))
    {
        const int nready = epoll_wait(epfd, events, static_cast<int>(DOFS), EPOLL_TIMEOUT_MS);

        if (nready < 0)
        {
            if (errno == EINTR) continue;
            std::cerr << "[JointStatePublisher] epoll_wait: " << strerror(errno) << '\n';
            break;
        }
        if (nready == 0)
            continue;

        bool any_new = false;

        for (int e = 0; e < nready; ++e)
        {
            const size_t joint = events[e].data.u32;
            if (joint >= DOFS) continue;

            // Drain the pipe — keep only the latest frame
            ssize_t last = 0;
            while (true)
            {
                ssize_t r = ::read(reader.fd(joint), raw, sizeof(raw));
                if (r <= 0) break;
                last = r;
            }

            if (last <= 0)
                continue;

            double pos = 0.0, vel = 0.0, eff = 0.0;
            uint64_t hw_ts = 0;
            if (pdo_utils::parse_joint_frame(raw, last, pos, vel, eff, hw_ts))
            {
                msg.positions[joint]  = pos;
                msg.velocities[joint] = vel;
                msg.efforts[joint]    = eff;
                if (!any_new) msg.header.timestamp_ns = hw_ts;
                any_new = true;
            }
        }

        if (any_new) {
            msg.header.seq = seq++;
            pub.publish(msg);
        }
    }

    ::close(epfd);
    std::cout << "[JointStatePublisher] exited.\n";
}

} 

// Entry points
void dds_publish_loop_spot_real(int domain_id) {
    run_real_dds_loop<SpotRobot, SpotJointStateMsg>(spot, domain_id);
}
void dds_publish_loop_ur5_real(int domain_id) {
    run_real_dds_loop<UR5Robot, UR5JointStateMsg>(ur5, domain_id);
}