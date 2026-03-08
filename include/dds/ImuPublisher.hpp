#pragma once

#include "dds/DdsPublisher.hpp"
#include "msgs/ImuMsg.hpp"
#include "sensor_msgs/msg/ImuPubSubTypes.hpp"

#include <cstring>
#include <iostream>

class ImuPublisher : public DdsPublisher<ImuPublisher>
{
    using Base = DdsPublisher<ImuPublisher>;
    friend Base; 

public:
    ImuPublisher()
        : Base(TypeSupport{new sensor_msgs::msg::ImuPubSubType()})
    {}

    ~ImuPublisher() = default;

    bool init(const std::string& frame_id = "", int domain_id = 0)
    {
        if (!Base::init_dds("rt/imu", "ImuPublisher", domain_id))
            return false;

        imu_.header().frame_id() = frame_id;
        return true;
    }

    void set_frame_id(const std::string& frame_id)
    {
        imu_.header().frame_id() = frame_id;
    }

    void publish(const rt_imu_msg& msg)
    {
        if (!Base::has_subscribers())
            return;

        // Timestamp
        imu_.header().stamp().sec()     = static_cast<int32_t>(msg.header.timestamp_ns / 1'000'000'000ULL);
        imu_.header().stamp().nanosec() = static_cast<uint32_t>(msg.header.timestamp_ns % 1'000'000'000ULL);

        // Orientation
        imu_.orientation().x() = msg.orientation.x;
        imu_.orientation().y() = msg.orientation.y;
        imu_.orientation().z() = msg.orientation.z;
        imu_.orientation().w() = msg.orientation.w;

        // Angular velocity
        imu_.angular_velocity().x() = msg.angular_velocity.x;
        imu_.angular_velocity().y() = msg.angular_velocity.y;
        imu_.angular_velocity().z() = msg.angular_velocity.z;

        // Linear acceleration
        imu_.linear_acceleration().x() = msg.linear_acceleration.x;
        imu_.linear_acceleration().y() = msg.linear_acceleration.y;
        imu_.linear_acceleration().z() = msg.linear_acceleration.z;

        writer_->write(&imu_);
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
        std::cout << "[ImuPublisher] Subscribers matched: " << matched_count << '\n';
    }

private:
    sensor_msgs::msg::Imu imu_;
};