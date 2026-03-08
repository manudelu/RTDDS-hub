#include "rt/SharedBuffer.hpp"

std::atomic<bool> g_running{true};

// RT -> DDS ring buffers (SPSC, lock-free)
boost::lockfree::spsc_queue<
    SpotJointStateMsg,
    boost::lockfree::capacity<RING_CAPACITY>> g_spot_ring;

boost::lockfree::spsc_queue<
    UR5JointStateMsg,
    boost::lockfree::capacity<RING_CAPACITY>> g_ur5_ring;