#include "dds/ImuPublisher.hpp"
#include "protobuf/pdo_utils.hpp"
#include "rt/SharedBuffer.hpp"
#include "rt/xddp_reader.hpp"
#include "msgs/ImuMsg.hpp"

#include <sys/epoll.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

namespace {

static constexpr int EPOLL_TIMEOUT_MS = 5;

void run_imu_dds_loop(const char* pipe_name, int domain_id)
{
    // Open the XDDP pipe
    XddpReader<1> reader;
    reader.open(&pipe_name, /*optional=*/true);

    // Set up epoll
    const int epfd = epoll_create1(0);
    if (epfd < 0)
    {
        std::cerr << "[ImuPublisher] epoll_create1: " << strerror(errno) << '\n';
        return;
    }

    if (reader.is_open(0))
    {
        struct epoll_event ev{};
        ev.events   = EPOLLIN;
        ev.data.u32 = 0;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, reader.fd(0), &ev) < 0)
        {
            std::cerr << "[ImuPublisher] epoll_ctl: " << strerror(errno) << '\n';
            ::close(epfd);
            return;
        }
    }

    // Initialise DDS publisher
    ImuPublisher pub;
    if (!pub.init(/*frame_id=*/"", domain_id))
    {
        std::cerr << "[ImuPublisher] failed to initialise publisher\n";
        ::close(epfd);
        return;
    }

    std::cout << "[ImuPublisher] Loop started (domain id " << domain_id << ")\n";

    rt_imu_msg msg{};
    uint64_t seq = 0;
    alignas(8) uint8_t raw[512];
    struct epoll_event events[1];

    while (g_running.load(std::memory_order_relaxed))
    {
        const int nready = epoll_wait(epfd, events, 1, EPOLL_TIMEOUT_MS);
        if (nready < 0) {
            if (errno == EINTR) continue;
            std::cerr << "[ImuPublisher] epoll_wait: " << strerror(errno) << '\n';
            break;
        }
        if (nready == 0) 
            continue;
        
        // Drain the pipe — keep only the latest frame
        ssize_t last = 0;
        while (true)
        {
            ssize_t r = ::read(reader.fd(0), raw, sizeof(raw));
            if (r <= 0) break;
            last = r;
        }

        if (last <= 0)
            continue;

        if (!pdo_utils::parse_imu_frame(raw, last, msg))
            continue;

        msg.header.seq = seq++;
        pub.publish(msg);
    }

    ::close(epfd);
    std::cout << "[ImuPublisher] exited.\n";
}

}

void dds_publish_loop_spot_imu(int domain_id) {
    run_imu_dds_loop(spot.imu_pipe_name, domain_id);
}

void dds_publish_loop_ur5_imu(int domain_id) {
    run_imu_dds_loop(ur5.imu_pipe_name, domain_id);
}