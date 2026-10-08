import os
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    package_name = 'srf08_publisher' 

    srf08_node = Node(
        package=package_name,
        executable='srf08_node',
        name='srf08_publisher_node',
        output='screen',
        parameters=[{
            'i2c_device': '/dev/i2c-1',
            'i2c_address': 0x73,  # passes sensor's I2C address to the node
            'frame_id': 'ultrasonic_sensor_link'
        }],
        remappings=[
            ('ultrasonic/range', 'scan/ultrasonic') 
        ]
    )

    return LaunchDescription([
        srf08_node
    ])
