#include "dds/JointStatePublisher.hpp"
#include "rt/SharedBuffer.hpp"
#include "robots/RobotModel.hpp"
#include "msgs/JointStateMsg.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {
    template <typename RobotT,
            typename MsgT,
            boost::lockfree::spsc_queue<MsgT, boost::lockfree::capacity<RING_CAPACITY>>& Ring>
    void run_sim_dds_loop(const RobotT& robot, int domain_id)
    {
        constexpr size_t DOFS = RobotT::dofs;
        const std::vector<std::string> joint_names(robot.joint_names, robot.joint_names + DOFS);

        JointStatePublisher<DOFS> pub;
        if (!pub.init(joint_names, domain_id))
        {
            std::cerr << "[dds_sim] Failed to initialise publisher for " << robot.joint_names[0] << '\n';
            g_running.store(false, std::memory_order_relaxed);
            return;
        }

        std::cout << "[dds_sim] Publisher loop started (" << DOFS << " DOF, "
                << "domain " << domain_id << ")\n";

        MsgT msg{};
        while (g_running.load(std::memory_order_relaxed))
        {
            if (Ring.pop(msg)) 
                pub.publish(msg);   
            else
                std::this_thread::sleep_for(std::chrono::microseconds(500));
        }

        std::cout << "[dds_sim] Publisher loop exited.\n";
    }
}

// Entry points
void dds_publish_loop_spot_sim(int domain_id) {
    run_sim_dds_loop<SpotRobot, SpotJointStateMsg, g_spot_ring>(spot, domain_id);
}
void dds_publish_loop_ur5_sim(int domain_id) {
    run_sim_dds_loop<UR5Robot, UR5JointStateMsg, g_ur5_ring>(ur5, domain_id);
}