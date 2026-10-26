import os
import random
import subprocess

import rclpy
from ament_index_python.packages import get_package_share_directory
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import Range
from std_msgs.msg import Bool


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
        self.is_playing = False
        self.last_played = self.get_clock().now()
        self.last_choice = None

        self.playing_publisher = self.create_publisher(Bool, '/speaker/playing', 10)
        self.playback_timer = self.create_timer(0.05, self.check_playback)
        self.create_subscription(
            Range, '/scan/ultrasonic', self.on_range, qos_profile_sensor_data)
        self.get_logger().info('Speaker node ready, listening on /scan/ultrasonic')

    def pick_sound(self):
        # Avoid playing the same file twice in a row
        choices = [i for i in range(1, 7) if i != self.last_choice]
        self.last_choice = random.choice(choices)
        return os.path.join(self.sounds_dir, f'{self.last_choice}.mp3')

    def set_playing(self, playing):
        if playing == self.is_playing:
            return
        self.is_playing = playing
        msg = Bool()
        msg.data = playing
        self.playing_publisher.publish(msg)

    def check_playback(self):
        if self.proc is not None and self.proc.poll() is not None:
            self.set_playing(False)

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
        try:
            self.proc = subprocess.Popen(
                ['pw-play', wav], stdout=subprocess.DEVNULL)
            self.last_played = self.get_clock().now()
            self.set_playing(True)
        except OSError as error:
            self.get_logger().error(f'Could not start pw-play for {wav}: {error}')

    def destroy_node(self):
        if self.proc is not None and self.proc.poll() is None:
            self.proc.terminate()
        self.set_playing(False)
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