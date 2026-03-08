#pragma once

#include "msgs/ImuMsg.hpp"
#include "msgs/JointStateMsg.hpp"

#include <ecat_pdo.pb.h>

#include <cstring>
#include <iostream>
#include <ctime>

/**
 * pdo_utils.hpp -- helpers for parsing length-prefixed protobuf PDO frames.
 *
 * Frame layout:
 *   bytes [0..3]  -- uint32_t little-endian payload length
 *   bytes [4..N]  -- serialised iit::advr::Ec_slave_pdo
 */
namespace pdo_utils {

inline bool parse_frame(const uint8_t* buf, ssize_t n,
                        iit::advr::Ec_slave_pdo& pdo_out)
{
    // First 4 bytes must be the payload length
    if (n < 4)
        return false;

    // Read the length prefix and check it matches the actual payload size
    uint32_t pb_len = 0;
    std::memcpy(&pb_len, buf, 4);
    if (static_cast<ssize_t>(pb_len + 4) != n)
        return false;

    return pdo_out.ParseFromArray(buf + 4, static_cast<int>(pb_len));
}

inline uint64_t realtime_now_ns()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<uint64_t>(ts.tv_sec)  * 1'000'000'000ULL
         + static_cast<uint64_t>(ts.tv_nsec);
}

inline uint64_t extract_timestamp_ns(const iit::advr::Ec_slave_pdo& pdo)
{
    if (pdo.has_header() && pdo.header().has_stamp())
    {
        const auto& s = pdo.header().stamp();
        return static_cast<uint64_t>(s.sec())  * 1'000'000'000ULL
             + static_cast<uint64_t>(s.nsec());
    }
    // Fallback to current time if no timestamp is present in the PDO
    return realtime_now_ns();
}

// Parses a PDO frame that carries IMU data (RX_IMU_VN)
inline bool parse_imu_frame(const uint8_t* buf, ssize_t n, rt_imu_msg& msg)
{
    iit::advr::Ec_slave_pdo pdo;
    if (!parse_frame(buf, n, pdo))
        return false;

    if (pdo.type() != iit::advr::Ec_slave_pdo::RX_IMU_VN || !pdo.has_imuvn_rx_pdo())
    {
        std::cerr << "[pdo_utils] unexpected PDO type (expected IMU_VN)=" << pdo.type() << '\n';
        return false;
    }

    const auto& rx = pdo.imuvn_rx_pdo();
    msg.header.timestamp_ns = extract_timestamp_ns(pdo);
    msg.orientation = {rx.x_quat(), rx.y_quat(), rx.z_quat(), rx.w_quat()};
    msg.angular_velocity = {rx.x_rate(), rx.y_rate(), rx.z_rate()};
    msg.linear_acceleration = {rx.x_acc(),  rx.y_acc(),  rx.z_acc()};

    return true;
}

// Parses a PDO frame that carries joint state data (CIA402, XT_MOTOR)
inline bool parse_joint_frame(const uint8_t* buf, ssize_t n,
                               double& pos, double& vel, double& eff,
                               uint64_t& timestamp_ns)
{
    iit::advr::Ec_slave_pdo pdo;
    if (!parse_frame(buf, n, pdo))\
        return false;
    
    timestamp_ns = extract_timestamp_ns(pdo);
        
    // Different drives have different PDO formats, so we need to check the type and parse accordingly
    if (pdo.type() == iit::advr::Ec_slave_pdo::RX_CIA402 && pdo.has_cia402_rx_pdo())
    {
        const auto& rx = pdo.cia402_rx_pdo();
        pos = rx.link_pos();
        vel = rx.link_vel();
        eff = rx.torque();
        return true;
    }

    if (pdo.type() == iit::advr::Ec_slave_pdo::RX_XT_MOTOR && pdo.has_motor_xt_rx_pdo())
    {
        const auto& rx = pdo.motor_xt_rx_pdo();
        pos = rx.link_pos();
        vel = rx.link_vel();
        eff = rx.torque();
        return true;
    }

    std::cerr << "[pdo_utils] unexpected PDO type=" << pdo.type() << '\n';
    return false;
}

} 