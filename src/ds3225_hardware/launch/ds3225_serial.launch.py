from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
import os


def generate_launch_description():
    # Launch the Arduino serial fallback with its own parameter file.
    parameters = os.path.join(
        get_package_share_directory("ds3225_hardware"),
        "config",
        "serial.yaml",
    )
    return LaunchDescription(
        [
            Node(
                package="ds3225_hardware",
                executable="ds3225_serial_bridge",
                name="ds3225_serial_bridge",
                parameters=[parameters],
                output="screen",
            )
        ]
    )
