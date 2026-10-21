import os
import subprocess
import random

import rclpy
from ament_index_python.packages import get_package_share_directory
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import Range


class SpeakerNode(Node):
    def __init__(self):
        super().__init__('speaker_node')


        rand = random.randint(0, 5)

        default_wav = os.path.join(
            get_package_share_directory('speaker_controller'),
            'sounds', f'{rand}.wav')


        
        self.declare_parameter('wav_path', default_wav)
        self.declare_parameter('trigger_distance_m', 0.5)
        self.declare_parameter('cooldown_s', 5.0)

        self.proc = None
        self.last_played = self.get_clock().now()
        self.create_subscription(
            Range, '/scan/ultrasonic', self.on_range, qos_profile_sensor_data)
        self.get_logger().info('Speaker node ready, listening on /scan/ultrasonic')

    def on_range(self, msg: Range):
        trigger = self.get_parameter('trigger_distance_m').value
        cooldown = self.get_parameter('cooldown_s').value
        wav = self.get_parameter('wav_path').value

        if not (msg.min_range <= msg.range <= trigger):
            return
        if self.proc is not None and self.proc.poll() is None:
            return  # still playing
        elapsed = (self.get_clock().now() - self.last_played).nanoseconds / 1e9
        if elapsed < cooldown:
            return

        self.get_logger().info(f'Object at {msg.range:.2f} m, playing sound')
        self.proc = subprocess.Popen(
            ['pw-play', wav],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        self.last_played = self.get_clock().now()

    def destroy_node(self):
        if self.proc is not None and self.proc.poll() is None:
            self.proc.terminate()
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = SpeakerNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()