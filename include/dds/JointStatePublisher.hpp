#pragma once

#include "dds/DdsPublisher.hpp"
#include "msgs/JointStateMsg.hpp"
#include "sensor_msgs/msg/JointStatePubSubTypes.hpp"

#include <iostream>
#include <string>
#include <vector>

template <size_t NUM_DOFS>
class JointStatePublisher : public DdsPublisher<JointStatePublisher<NUM_DOFS>>
{
    using Base    = DdsPublisher<JointStatePublisher<NUM_DOFS>>;
    using MsgType = rt_joint_state_msg<NUM_DOFS>;
    friend Base;  

public:
    JointStatePublisher()
        : Base(TypeSupport{new sensor_msgs::msg::JointStatePubSubType()})
    {}

    ~JointStatePublisher() = default;

    bool init(const std::vector<std::string>& joint_names, int domain_id = 0)
    {
        if (joint_names.size() != NUM_DOFS)
        {
            std::cerr << "[JointStatePublisher] joint_names size "
                      << joint_names.size() << " != NUM_DOFS " << NUM_DOFS << '\n';
            return false;
        }

        if (!Base::init_dds("rt/joint_states", "JointStatePublisher", domain_id))
            return false;

        joint_state_.name() = joint_names;
        joint_state_.position().assign(NUM_DOFS, 0.0);
        joint_state_.velocity().assign(NUM_DOFS, 0.0);
        joint_state_.effort().assign(NUM_DOFS, 0.0);
        joint_state_.header().frame_id() = "";

        return true;
    }

    void publish(const MsgType& msg)
    {
        if (!Base::has_subscribers())
            return;

        // Timestamp
        joint_state_.header().stamp().sec()     = static_cast<int32_t>(msg.header.timestamp_ns / 1'000'000'000ULL);
        joint_state_.header().stamp().nanosec() = static_cast<uint32_t>(msg.header.timestamp_ns % 1'000'000'000ULL);

        // Joint data
        for (std::size_t i = 0; i < NUM_DOFS; ++i)
        {
            joint_state_.position()[i] = msg.positions[i];
            joint_state_.velocity()[i] = msg.velocities[i];
            joint_state_.effort()[i]   = msg.efforts[i];
        }

        Base::writer_->write(&joint_state_);
    }

    static DataWriterQos writer_qos()
    {
        DataWriterQos qos;
        qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
        qos.history().kind = KEEP_LAST_HISTORY_QOS;
        qos.history().depth = 1;
        qos.resource_limits().max_samples = 1;
        qos.resource_limits().allocated_samples = 1;
        return qos;
    }

    void on_matched(int matched_count)
    {
        std::cout << "[JointStatePublisher] Subscribers matched: " << matched_count << '\n';
    }

private:
    sensor_msgs::msg::JointState joint_state_;
};