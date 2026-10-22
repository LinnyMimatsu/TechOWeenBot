from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    # Launch the GPIO controller with the package's servo configuration.
    parameters = os.path.join(
        get_package_share_directory("ds3225_hardware"),
        "config",
        "servos.yaml",
    )
    return LaunchDescription(
        [
            Node(
                package="ds3225_hardware",
                executable="ds3225_gpio_node",
                name="ds3225_gpio_node",
                parameters=[parameters],
                output="screen",
            )
        ]
    )
