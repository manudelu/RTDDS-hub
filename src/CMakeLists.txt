set(XBOT2_PB_DIR  /home/advrf_advtech/xbot2_ws/build/ecat_master/protobuf)
set(XBOT2_INSTALL /home/advrf_advtech/xbot2_ws/install)

find_package(Xenomai  REQUIRED)
find_package(fastcdr  2 REQUIRED)
find_package(fastdds  3 REQUIRED)

# Fast-DDS type-support sources generated from IDL
file(GLOB_RECURSE DDS_GENERATED_SOURCES
    "${CMAKE_SOURCE_DIR}/generated/**/*.cxx"
    "${CMAKE_SOURCE_DIR}/generated/**/*.cxx"
)

add_executable(JointStatePublisher
    # main
    rtdds_hub_main.cpp
    # publisher files (joint state publisher + real robot)
    joint_state_dds_real.cpp
    imu_dds_real.cpp
    # helper files        
    rt_sockets.cpp 
    # only-simulation files (traj + sim)
    rt_trajectory_client.cpp 
    joint_state_dds_sim.cpp    
    shared_buffer.cpp   
    ${DDS_GENERATED_SOURCES}
)
target_include_directories(JointStatePublisher PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/generated
    ${CMAKE_SOURCE_DIR}/generated
    ${XBOT2_PB_DIR}
)
target_compile_options(JointStatePublisher PRIVATE
    ${XENOMAI_CFLAGS}
)
target_link_libraries(JointStatePublisher PRIVATE
    Xenomai::POSIX
    fastdds
    fastcdr
    ${XBOT2_PB_DIR}/libecat_master_pb.a
    ${XBOT2_INSTALL}/lib/libprotobuf.a
    pthread
    z
)
install(TARGETS JointStatePublisher
    RUNTIME DESTINATION bin
)
