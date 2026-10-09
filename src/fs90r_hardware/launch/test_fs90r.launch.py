
from launch import LaunchDescription
from launch.actions import TimerAction
from launch.substitutions import Command, PathJoinSubstitution, FindExecutable
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    description_file = PathJoinSubstitution([
        FindPackageShare('fs90r_hardware'),
        'urdf',
        'fs90r_control.xacro'
    ])

    controllers_file = PathJoinSubstitution([
        FindPackageShare('fs90r_hardware'),
        'config',
        'controllers.yaml'
    ])

    robot_description = ParameterValue(
        Command([
            FindExecutable(name='xacro'),
            ' ',
            description_file
        ]),
        value_type=str
    )

    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{'robot_description': robot_description}],
        output='screen'
    )

    control_node = Node(
        package='controller_manager',
        executable='ros2_control_node',
        parameters=[controllers_file],
        output='screen'
    )

    joint_state_spawner = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['joint_state_broadcaster'],
        output='screen'
    )

    velocity_spawner = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['forward_velocity_controller'],
        output='screen'
    )

    return LaunchDescription([
        robot_state_publisher,
        control_node,
        TimerAction(period=2.0, actions=[joint_state_spawner]),
        TimerAction(period=3.0, actions=[velocity_spawner]),
    ])
