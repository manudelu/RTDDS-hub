#include "rt/SharedBuffer.hpp"
#include "rt/rt_client.hpp"
#include "robots/RobotModel.hpp"
#include "dds/entry_points.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <signal.h>
#include <pthread.h>
#include <stdexcept>
#include <string> 

static void on_shutdown(int)
{
    g_running.store(false, std::memory_order_relaxed);
}

// Struct to hold arguments for NRT thread entry point
struct NrtArgs
{
    void (*fn)(int domain_id); 
    int domain_id;
};

static void* nrt_thread_entry(void* raw)
{
    auto* args = static_cast<NrtArgs*>(raw);
    auto fn = args->fn;
    int domain = args->domain_id;
    delete args;   
    fn(domain);
    return nullptr;
}

static void* wrap_client_spot(void*) { return client_spot(nullptr); }
static void* wrap_client_ur5 (void*) { return client_ur5(nullptr);  }

static bool spawn_rt_thread(pthread_t& t, void* (*fn)(void*), int priority, const char* name)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    struct sched_param p = {priority};
    pthread_attr_setschedparam(&attr, &p);

    const bool ok = (pthread_create(&t, &attr, fn, nullptr) == 0);
    pthread_attr_destroy(&attr);

    if (!ok)
        std::cerr << "[main] could not create RT thread '" << name
                  << "' (prio " << priority << "): " << strerror(errno) << '\n';
    return ok;
}

static bool spawn_nrt_thread(pthread_t& t, void (*fn)(int), int domain_id, const char* name)
{
    auto* args = new NrtArgs{fn, domain_id};

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
    struct sched_param p = {0};
    pthread_attr_setschedparam(&attr, &p);

    const bool ok = (pthread_create(&t, &attr, nrt_thread_entry, args) == 0);
    pthread_attr_destroy(&attr);

    if (!ok)
    {
        delete args;   
        std::cerr << "[main] could not create NRT thread '" << name
                  << "': " << strerror(errno) << '\n';
    }
    return ok;
}

enum class Mode { Sim, Real };

struct RobotConfig
{
    const char*  name;
    void* (*rt_fn)(void*);   
    void  (*sim_fn)(int);   
    void  (*real_fn)(int);  
    void  (*imu_fn)(int);
};

static constexpr int RT_PRIORITY = 90;
static constexpr int DEFAULT_DOMAIN =  0;

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <robot> <mode> [domain_id]\n"
                  << "  robot     : spot | ur5\n"
                  << "  mode      : sim  | real\n"
                  << "  domain_id : integer (default = " << DEFAULT_DOMAIN << ")\n";
        return 1;
    }

    // Parse mode 
    const std::string mode_arg(argv[2]);
    Mode mode;
    if (mode_arg == "sim")  
        mode = Mode::Sim;
    else if 
        (mode_arg == "real") mode = Mode::Real;
    else 
    { 
        std::cerr << "[main] unknown mode '" << mode_arg << "'\n"; 
        return 1; 
    }

    // Parse robot 
    const std::string robot_arg(argv[1]);
    RobotConfig cfg;
    if (robot_arg == "spot")
        cfg = {"Spot", 
               wrap_client_spot, 
               dds_publish_loop_spot_sim, 
               dds_publish_loop_spot_real,
               spot.imu_pipe_name ? dds_publish_loop_spot_imu : nullptr
            };
    else if (robot_arg == "ur5")
        cfg = {"UR5",  
               wrap_client_ur5, 
               dds_publish_loop_ur5_sim,  
               dds_publish_loop_ur5_real,
               ur5.imu_pipe_name ? dds_publish_loop_ur5_imu : nullptr
            };
    else 
    { 
        std::cerr << "[main] unknown robot '" << robot_arg << "'\n"; 
        return 1; 
    }

    // Parse domain_id
    int domain_id = DEFAULT_DOMAIN;
    if (argc >= 4)
    {
        try 
        {
            domain_id = std::stoi(argv[3]); 
        }
        catch (const std::exception&)
        {
            std::cerr << "[main] invalid domain_id '" << argv[3] << "'\n";
            return 1;
        }
    }

    std::cout << "=== RTDDS-Hub  robot=" << cfg.name
              << "  mode=" << mode_arg
              << "  domain=" << domain_id << " ===\n";

    signal(SIGINT,  on_shutdown);
    signal(SIGTERM, on_shutdown);

    //Launch threads 
    if (mode == Mode::Sim)
    {
        pthread_t dds_t, rt_t;

        if (!spawn_nrt_thread(dds_t, cfg.sim_fn, domain_id, "dds_sim"))
            return 1;

        if (!spawn_rt_thread(rt_t, cfg.rt_fn, RT_PRIORITY, "rt_client"))
        {
            g_running.store(false, std::memory_order_relaxed);
            pthread_join(dds_t, nullptr);
            return 1;
        }

        std::cout << "[main] running — Ctrl+C to stop\n\n";
        pthread_join(rt_t,  nullptr);
        pthread_join(dds_t, nullptr);
    }
    else // Mode::Real
    {
        pthread_t dds_t;

        if (!spawn_nrt_thread(dds_t, cfg.real_fn, domain_id, "JointStatePublisher"))
            return 1;
        
        pthread_t imu_t{};
        bool has_imu = false;
        if (cfg.imu_fn)
        {
            if (!spawn_nrt_thread(imu_t, cfg.imu_fn, domain_id, "ImuPublisher"))
                std::cerr << "[main] IMU thread failed to start, continuing without IMU\n";
            else
                has_imu = true;
        }

        std::cout << "[main] running — Ctrl+C to stop\n\n";
        pthread_join(dds_t, nullptr);
        if (has_imu)
            pthread_join(imu_t, nullptr);
    }

    std::cout << "[main] shutdown complete\n";
    return 0;
}