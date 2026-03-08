#pragma once

// Forward declarations for all DDS publish loop entry points.
// Each function blocks until g_running becomes false.

// Real DDS loops (read from XDDP pipes, publish to DDS)
void dds_publish_loop_spot_real(int domain_id);
void dds_publish_loop_ur5_real (int domain_id);
void dds_publish_loop_spot_imu (int domain_id);
void dds_publish_loop_ur5_imu  (int domain_id);


// Sim DDS loops (publish fake sine wave data to DDS)
void dds_publish_loop_spot_sim (int domain_id);
void dds_publish_loop_ur5_sim  (int domain_id);