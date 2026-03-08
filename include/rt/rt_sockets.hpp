#pragma once
#include <rtdm/ipc.h>

/**
 *
 * Port label naming convention (mirrors EtherCAT PDO pattern):
 *   {prefix}/{protocol}/{name}
 *
 *   e.g.  /proc/xenomai/registry/rtipc/xddp/None@Synap_Motor_id_1 -> /dev/rtp4
 *         /proc/xenomai/registry/rtipc/iddp/None@Motor_id_1_rx_pdo
*/
#define IDDP_PORT_LABEL "None@iddp-joint-state"
#define XDDP_PORT_LABEL "None@xddp-joint-state"

enum class RtSocketRole { Server, Client };

int create_rt_socket(const char* label, bool is_iddp, RtSocketRole role);
int open_xddp_device(const char* label, int flags);
