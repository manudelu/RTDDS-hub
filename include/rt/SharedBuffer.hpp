#pragma once

#include <atomic>
#include <boost/lockfree/spsc_queue.hpp>
#include "msgs/JointStateMsg.hpp"

static constexpr std::size_t RING_CAPACITY = 8;

// Set to false to signal all threads to exit cleanly
extern std::atomic<bool> g_running;

// Spot RT -> DDS ring buffer (SPSC, sim mode)
extern boost::lockfree::spsc_queue<
    SpotJointStateMsg,
    boost::lockfree::capacity<RING_CAPACITY>> g_spot_ring;

// UR5 RT -> DDS ring buffer (SPSC, sim mode)
extern boost::lockfree::spsc_queue<
    UR5JointStateMsg,
    boost::lockfree::capacity<RING_CAPACITY>> g_ur5_ring;