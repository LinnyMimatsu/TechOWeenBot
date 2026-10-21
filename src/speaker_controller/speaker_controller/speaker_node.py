import os
import random
import subprocess

import rclpy
from ament_index_python.packages import get_package_share_directory
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import Range


class SpeakerNode(Node):
    def __init__(self):
        super().__init__('speaker_node')

        self.sounds_dir = os.path.join(
            get_package_share_directory('speaker_controller'), 'sounds')

        # Empty string = pick randomly each time. Set it to force one file.
        self.declare_parameter('wav_path', '')
        self.declare_parameter('trigger_distance_m', 0.5)
        self.declare_parameter('cooldown_s', 5.0)

        self.proc = None
        self.last_played = self.get_clock().now()
        self.last_choice = None

        self.create_subscription(
            Range, '/scan/ultrasonic', self.on_range, qos_profile_sensor_data)
        self.get_logger().info('Speaker node ready, listening on /scan/ultrasonic')

    def pick_sound(self):
        # Avoid playing the same file twice in a row
        choices = [i for i in range(1, 6) if i != self.last_choice]
        self.last_choice = random.choice(choices)
        return os.path.join(self.sounds_dir, f'{self.last_choice}.mp3')

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

        if not wav:
            wav = self.pick_sound()

        self.get_logger().info(f'Object at {msg.range:.2f} m, playing {wav}')
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