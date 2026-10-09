
#include <chrono>
#include <cmath>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/range.hpp"
#include "std_msgs/msg/float64_multi_array.hpp"

using namespace std::chrono_literals;

class RangeServoController : public rclcpp::Node
{
public:
  RangeServoController()
  : Node("range_servo_controller")
  {
    command_pub_ =
      create_publisher<std_msgs::msg::Float64MultiArray>(
        "/forward_velocity_controller/commands", 10);

    range_sub_ = create_subscription<sensor_msgs::msg::Range>(
      "/scan/ultrasonic",
      10,
      [this](const sensor_msgs::msg::Range::SharedPtr msg)
      {
        last_message_time_ = std::chrono::steady_clock::now();
        received_message_ = true;

        const double distance = msg->range;

        // Invalid readings or no detected object: stop.
        if (!std::isfinite(distance) ||
            distance >= msg->max_range ||
            distance < msg->min_range)
        {
          active_ = false;
          return;
        }

        // Hysteresis: start inside 20 cm, stop outside 25 cm.
        if (distance <= 0.20) {
          active_ = true;
        } else if (distance >= 0.25) {
          active_ = false;
        }
      });

    // Publish commands continuously and stop if sensor data goes stale.
    timer_ = create_wall_timer(
      100ms,
      [this]()
      {
        const auto now = std::chrono::steady_clock::now();

        const bool sensor_timed_out =
          !received_message_ ||
          now - last_message_time_ > 500ms;

        const double command =
          (!sensor_timed_out && active_) ? 1.0 : 0.0;

        std_msgs::msg::Float64MultiArray msg;
        msg.data = {command};
        command_pub_->publish(msg);
      });

    RCLCPP_INFO(
      get_logger(),
      "Range servo controller started; default command is STOP (0.0)");
  }

private:
  rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr
    command_pub_;

  rclcpp::Subscription<sensor_msgs::msg::Range>::SharedPtr
    range_sub_;

  rclcpp::TimerBase::SharedPtr timer_;

  bool active_{false};
  bool received_message_{false};

  std::chrono::steady_clock::time_point last_message_time_{};
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<RangeServoController>());
  rclcpp::shutdown();
  return 0;
}