#include "rt/rt_sockets.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

static void error_handler(const char* context)
{
    perror(context);
    exit(EXIT_FAILURE);
}

int create_rt_socket(const char* label, bool is_iddp, RtSocketRole role)
{
    const int proto   = is_iddp ? IPCPROTO_IDDP   : IPCPROTO_XDDP;
    const int optname = is_iddp ? IDDP_LABEL      : XDDP_LABEL;
    const int sol     = is_iddp ? SOL_IDDP        : SOL_XDDP;

    int sock = socket(AF_RTIPC, SOCK_DGRAM, proto);
    if (sock < 0)
        error_handler("socket");

    struct rtipc_port_label port_label;
    memset(&port_label, 0, sizeof(port_label));
    strncpy(port_label.label, label, sizeof(port_label.label) - 1);

    if (setsockopt(sock, sol, optname, &port_label, sizeof(port_label)) < 0)
        error_handler("setsockopt");

    struct sockaddr_ipc addr;
    memset(&addr, 0, sizeof(addr));
    addr.sipc_family = AF_RTIPC;
    addr.sipc_port   = -1;  // -1 → kernel assigns a dynamic port

    if (role == RtSocketRole::Server)
    {
        if (bind(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
            error_handler("bind");
    }
    else
    {
        if (connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
            error_handler("connect");
    }

    return sock;
}

int open_xddp_device(const char* label, int flags)
{
    char* path = nullptr;
    if (asprintf(&path, "/proc/xenomai/registry/rtipc/xddp/%s", label) < 0)
        return -1;

    int fd = open(path, flags);
    free(path);

    return fd;
}
