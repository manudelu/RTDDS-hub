# RTDDS-Hub

## System Tested

- ROS2 Jazzy/Humble  
- Ubuntu 24.04/22.04  
- [FastDDS](https://fast-dds.docs.eprosima.com/en/stable/installation/sources/sources_linux.html)

## Running the Demo

### PC side: 

```bash
git clone https://github.com/Advanced-Robotics-Facility/ros2_ws.git --recursive
source /opt/ros/<distro>/setup.bash 
source ~/ros2_ws/install/setup.bash
ros2 launch robot_bringup robot.launch.py robot:=spot #Flags --> robot:=spot/ur type:=ur5 
```

`Note:` Make sure to set a ROS ID matching the one in DDS (default: 0)
```bash
export ROS_DOMAIN_ID=42
```

#### Optional: Check joint_state msg 
```bash
source /opt/ros/<distro>/setup.bash 
source ~/ros2_ws/install/setup.bash
ros2 topic echo /joint_states
```

### Embedded side:

```bash
source ./scripts/setup_fastdds.sh --clean
./RTDDS-Hub/build/src/JointStatePublisher spot real 42 # robot:=spot/ur5 mode:=sim/real id:=dds_domain_id
```

`Note:` If you want to add the Imu -> add the XDDP pipe name in `include/robots/RobotModel.hpp` and then use `ros2 topic echo /imu` in PC side.