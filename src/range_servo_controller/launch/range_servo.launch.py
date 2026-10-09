
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='range_servo_controller',
            executable='range_servo_controller',
            name='range_servo_controller',
            output='screen',
            remappings=[
                ('/ultrasonic/range', '/scan/ultrasonic'),
            ],
        ),
    ])