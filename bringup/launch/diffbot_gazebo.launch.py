import os

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, TimerAction, RegisterEventHandler
from launch.event_handlers import OnProcessExit
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command, FindExecutable, LaunchConfiguration, PathJoinSubstitution

from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():

    # Declare the 'prefix' launch argument
    declare_prefix_arg = DeclareLaunchArgument(
        "prefix",
        default_value="",
        description="Prefix for the robot name and for all joints and links"
    )

   # Process the xacro file to get the robot description 
    robot_description_content = Command(
        [
        PathJoinSubstitution([FindExecutable(name="xacro")]),
        " ",
        PathJoinSubstitution([FindPackageShare("diffdrive_esp32"), "description", "urdf", "diffbot_description.urdf.xacro"]),
        " prefix:=",
        LaunchConfiguration("prefix")
        ]
    )

    robot_description = {"robot_description": robot_description_content}


    # Path to the controller configuration file
    robot_controllers = PathJoinSubstitution(
        [FindPackageShare("diffdrive_esp32"), "bringup", "config", "diffbot_controllers.yaml"]
    )

    # Launch Gazebo with an empty world
    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare("gazebo_ros"), "launch", "gazebo.launch.py"
            ])
        ]),
        launch_arguments={"world": ""}.items()
    )

    # Spawn the robot into Gazebo using the robot_description parameter
    spawn_entity = Node(
        package="gazebo_ros",
        executable="spawn_entity.py",
        arguments=[
            "-topic", "robot_description",
            "-entity", "diffdrive_bot",
            "-x", "0",
            "-y", "0",
            "-z", "0.1"
        ],
        output="screen"
    )


    # Reads the robot description and joint states and publishes tf Tree
    robot_state_pub_node = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        output="both",
        parameters=[robot_description]
    )


    # joint state broadcaster: reads the joint states from GazeboSystem and publishes them to /joint_states topic
    joint_state_broadcaster_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["joint_state_broadcaster", "--controller-manager", "/controller_manager"]
    )

    # diffbot base controller: subscribes to /cmd_vel topic, publishes to /odom topic
    robot_controller_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["diffbot_base_controller", "--controller-manager", "/controller_manager"]
    )   


    # Delay controller spawner until Gazebo is fully up and running
    delay_joint_state_broadcaster = TimerAction(
        period=3.0,
        actions=[joint_state_broadcaster_spawner]
    )

    # Delay start of robot_controller after `joint_state_broadcaster`
    delay_robot_controller_after_broadcaster = RegisterEventHandler(
        event_handler=OnProcessExit(
            target_action=joint_state_broadcaster_spawner,
            on_exit=[robot_controller_spawner]
        )
    )   

    return LaunchDescription([
        declare_prefix_arg,
        gazebo,
        robot_state_pub_node,
        spawn_entity,
        delay_joint_state_broadcaster,
        delay_robot_controller_after_broadcaster
    ])
        





