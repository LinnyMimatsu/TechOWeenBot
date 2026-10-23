from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='speaker_controller',
            executable='speaker_node',
            name='speaker_node',
            output='screen',
            parameters=[{
                'trigger_distance_m': 0.2,
                'cooldown_s': 5.0,
            }],
        ),
    ])
