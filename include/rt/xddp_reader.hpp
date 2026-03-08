#pragma once
#include "rt/rt_sockets.hpp"
#include "robots/RobotModel.hpp"

#include <array>
#include <cstddef>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <string>

template <size_t NUM_DOFS>
class XddpReader
{
public:
    XddpReader() { fds_.fill(-1); }

    ~XddpReader()
    {
        for (int fd : fds_)
            if (fd >= 0) ::close(fd);
    }

    XddpReader(const XddpReader&) = delete;
    XddpReader& operator=(const XddpReader&) = delete;

    bool open(const char* const pipe_names[NUM_DOFS], bool optional = false)
    {
        for (size_t i = 0; i < NUM_DOFS; ++i)
        {
            fds_[i] = open_xddp_device(pipe_names[i], O_RDONLY | O_NONBLOCK);
            if (fds_[i] < 0)
            {
                if (optional)
                {
                    std::cerr << "[XddpReader] Pipe '" << pipe_names[i]
                          << "' not found — joint " << i << " will read as zero\n";
                    continue;
                }
                std::cerr << "[XddpReader] Failed to open pipe '"
                          << pipe_names[i] << "'\n";
                return false;
            }
            std::cout << "[XddpReader] Opened pipe '" << pipe_names[i]
                      << "' fd=" << fds_[i] << '\n';
        }
        return true;
    }

    int fd(size_t joint_idx) const { return fds_[joint_idx]; }
    bool is_open(size_t i) const { return fds_[i] >= 0; }

private:
    std::array<int, NUM_DOFS> fds_;
};