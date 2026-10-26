#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>

#include <lgpio.h>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/bool.hpp"
#include "std_msgs/msg/float64.hpp"

class DS3225GpioNode : public rclcpp::Node
{
public:
  // Load settings, initialize both GPIO outputs, and connect ROS topics/timers.
  DS3225GpioNode()
  : Node("ds3225_gpio_node")
  {
    const int gpio_chip = declare_parameter<int>("gpio_chip", 0);
    servo1_pin_ = declare_parameter<int>("servo1_gpio_pin", 5);
    servo2_pin_ = declare_parameter<int>("servo2_gpio_pin", 6);
    min_angle_ = declare_parameter<double>("min_angle", 0.0);
    max_angle_ = declare_parameter<double>("max_angle", 180.0);
    home_angle_ = declare_parameter<double>("home_angle", 0.0);
    jaw_open_angle_ = declare_parameter<double>("jaw_open_angle", 90.0);
    min_pulse_width_us_ = declare_parameter<int>("min_pulse_width_us", 500);
    max_pulse_width_us_ = declare_parameter<int>("max_pulse_width_us", 2500);
    pwm_frequency_hz_ = declare_parameter<int>("pwm_frequency_hz", 50);
    jaw_playback_topic_ = declare_parameter<std::string>(
      "jaw_playback_topic", "/speaker/playing");
    servo1_command_topic_ =
      declare_parameter<std::string>("servo1_command_topic", "/ds3225/angle");
    servo2_command_topic_ =
      declare_parameter<std::string>("servo2_command_topic", "/ds3225/servo2/angle");
    validate_parameters();

    gpio_chip_handle_ = lgGpiochipOpen(gpio_chip);
    if (gpio_chip_handle_ < 0) {
      throw std::runtime_error("lgpio could not open GPIO chip " + std::to_string(gpio_chip));
    }

    try {
      claim_gpio(servo1_pin_);
      claim_gpio(servo2_pin_);
      set_servo_angle(servo1_pin_, home_angle_);
      set_servo_angle(servo2_pin_, home_angle_);

      // Allow direct angle commands for servo 1.
      servo1_subscription_ = create_subscription<std_msgs::msg::Float64>(
        servo1_command_topic_,
        10,
        [this](const std_msgs::msg::Float64::SharedPtr msg) {
          set_servo_angle(servo1_pin_, msg->data);
        });

      // Allow direct angle commands for servo 2.
      servo2_subscription_ = create_subscription<std_msgs::msg::Float64>(
        servo2_command_topic_,
        10,
        [this](const std_msgs::msg::Float64::SharedPtr msg) {
          set_servo_angle(servo2_pin_, msg->data);
        });

      playback_subscription_ = create_subscription<std_msgs::msg::Bool>(
        jaw_playback_topic_,
        10,
        [this](const std_msgs::msg::Bool::SharedPtr msg) {
          const double angle = msg->data ? jaw_open_angle_ : home_angle_;
          set_servo_angle(servo1_pin_, angle);
        });

      RCLCPP_INFO(
        get_logger(),
        "DS3225 GPIO controller ready on BCM GPIO %d and %d; jaw playback from %s "
        "moves servo 1 between %.1f and %.1f degrees",
        servo1_pin_, servo2_pin_, jaw_playback_topic_.c_str(), home_angle_, jaw_open_angle_);
    } catch (...) {
      release_gpio();
      throw;
    }
  }

  // Stop PWM and release the GPIO resources when the node exits.
  ~DS3225GpioNode() override
  {
    release_gpio();
  }

private:
  // Reject unsafe or inconsistent GPIO, servo, and sensor settings.
  void validate_parameters() const
  {
    if (servo1_pin_ < 0 || servo1_pin_ > 27 ||
      servo2_pin_ < 0 || servo2_pin_ > 27 ||
      servo1_pin_ == servo2_pin_)
    {
      throw std::invalid_argument(
              "Servo GPIO pins must be distinct Raspberry Pi BCM GPIO numbers from 0 to 27");
    }
    if (!std::isfinite(min_angle_) || !std::isfinite(max_angle_) ||
      !std::isfinite(home_angle_) || !std::isfinite(jaw_open_angle_) ||
      min_angle_ < 0.0 || max_angle_ > 180.0 || min_angle_ >= max_angle_ ||
      home_angle_ < min_angle_ || home_angle_ > max_angle_ ||
      jaw_open_angle_ < min_angle_ || jaw_open_angle_ > max_angle_)
    {
      throw std::invalid_argument(
              "Servo angles must satisfy 0 <= min < max <= 180 and home/jaw-open angles "
              "must be within that range");
    }
    if (min_pulse_width_us_ < 500 ||
      max_pulse_width_us_ > 2500 ||
      min_pulse_width_us_ >= max_pulse_width_us_ || pwm_frequency_hz_ <= 0)
    {
      throw std::invalid_argument(
              "Pulse widths must satisfy 500 <= min < max <= 2500 microseconds, and PWM "
              "frequency must be positive");
    }
  }

  // Reserve one GPIO line as an output for a servo signal.
  void claim_gpio(int pin)
  {
    const int result = lgGpioClaimOutput(gpio_chip_handle_, 0, pin, 0);
    if (result < 0) {
      throw std::runtime_error(
              "lgpio could not claim BCM GPIO " + std::to_string(pin) + " as an output");
    }
    if (pin == servo1_pin_) {
      servo1_claimed_ = true;
    } else {
      servo2_claimed_ = true;
    }
  }

  // Stop PWM and release any GPIO lines claimed by this node.
  void release_gpio() noexcept
  {
    if (gpio_chip_handle_ < 0) {
      return;
    }

    if (servo1_claimed_) {
      lgTxServo(gpio_chip_handle_, servo1_pin_, 0, pwm_frequency_hz_, 0, 0);
      lgGpioWrite(gpio_chip_handle_, servo1_pin_, 0);
      lgGpioFree(gpio_chip_handle_, servo1_pin_);
      servo1_claimed_ = false;
    }
    if (servo2_claimed_) {
      lgTxServo(gpio_chip_handle_, servo2_pin_, 0, pwm_frequency_hz_, 0, 0);
      lgGpioWrite(gpio_chip_handle_, servo2_pin_, 0);
      lgGpioFree(gpio_chip_handle_, servo2_pin_);
      servo2_claimed_ = false;
    }
    lgGpiochipClose(gpio_chip_handle_);
    gpio_chip_handle_ = -1;
  }

  // Convert a requested angle to pulse width and start servo PWM on its GPIO.
  void set_servo_angle(int pin, double angle)
  {
    if (!std::isfinite(angle) || angle < min_angle_ || angle > max_angle_) {
      RCLCPP_WARN(
        get_logger(), "Ignoring invalid servo angle %.3f; expected %.1f..%.1f degrees",
        angle, min_angle_, max_angle_);
      return;
    }

    const int pulse_width_us = static_cast<int>(std::lround(
      min_pulse_width_us_ +
      ((angle - min_angle_) / (max_angle_ - min_angle_)) *
      (max_pulse_width_us_ - min_pulse_width_us_)));
    const int result = lgTxServo(
      gpio_chip_handle_, pin, pulse_width_us, pwm_frequency_hz_, 0, 0);
    if (result < 0) {
      RCLCPP_ERROR(
        get_logger(), "lgpio failed to set BCM GPIO %d to %.0f degrees", pin, angle);
      return;
    }

    RCLCPP_INFO(
      get_logger(), "BCM GPIO %d commanded to %.0f degrees (%d us pulse)",
      pin, angle, pulse_width_us);
  }

  int gpio_chip_handle_{-1};
  bool servo1_claimed_{false};
  bool servo2_claimed_{false};
  int servo1_pin_{5};
  int servo2_pin_{6};
  double min_angle_{0.0};
  double max_angle_{180.0};
  double home_angle_{0.0};
  double jaw_open_angle_{90.0};
  int min_pulse_width_us_{500};
  int max_pulse_width_us_{2500};
  int pwm_frequency_hz_{50};
  std::string jaw_playback_topic_{"/speaker/playing"};
  std::string servo1_command_topic_{"/ds3225/angle"};
  std::string servo2_command_topic_{"/ds3225/servo2/angle"};
  rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr servo1_subscription_;
  rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr servo2_subscription_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr playback_subscription_;
};

int main(int argc, char ** argv)
{
  // Initialize ROS, run the GPIO controller, and report startup/runtime failures.
  rclcpp::init(argc, argv);
  int result = 0;
  try {
    rclcpp::spin(std::make_shared<DS3225GpioNode>());
  } catch (const std::exception & error) {
    RCLCPP_FATAL(
      rclcpp::get_logger("ds3225_gpio_node"), "GPIO controller stopped: %s", error.what());
    result = 1;
  }
  rclcpp::shutdown();
  return result;
}
