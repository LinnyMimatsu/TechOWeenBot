#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>

#include <lgpio.h>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/range.hpp"
#include "std_msgs/msg/float64.hpp"

using namespace std::chrono_literals;

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
    object_detect_angle_ = declare_parameter<double>("object_detect_angle", 90.0);
    min_pulse_width_us_ = declare_parameter<int>("min_pulse_width_us", 500);
    max_pulse_width_us_ = declare_parameter<int>("max_pulse_width_us", 2500);
    pwm_frequency_hz_ = declare_parameter<int>("pwm_frequency_hz", 50);
    range_topic_ = declare_parameter<std::string>("range_topic", "/scan/ultrasonic");
    servo1_command_topic_ =
      declare_parameter<std::string>("servo1_command_topic", "/ds3225/angle");
    servo2_command_topic_ =
      declare_parameter<std::string>("servo2_command_topic", "/ds3225/servo2/angle");
    object_detect_distance_ = declare_parameter<double>("object_detect_distance", 0.20);
    object_clear_distance_ = declare_parameter<double>("object_clear_distance", 0.25);
    sensor_timeout_seconds_ = declare_parameter<double>("sensor_timeout_seconds", 0.5);
    sensor_check_period_ms_ = declare_parameter<int>("sensor_check_period_ms", 100);
    hold_duration_seconds_ = declare_parameter<double>("hold_duration_seconds", 2.0);

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

      // Use SRF08 distance messages to detect and clear nearby objects.
      range_subscription_ = create_subscription<sensor_msgs::msg::Range>(
        range_topic_,
        10,
        [this](const sensor_msgs::msg::Range::SharedPtr msg) {
          last_sensor_message_time_ = std::chrono::steady_clock::now();
          received_sensor_message_ = true;

          const double distance = msg->range;
          if (!std::isfinite(distance) ||
            distance < msg->min_range || distance >= msg->max_range)
          {
            set_object_detected(false);
          } else if (distance <= object_detect_distance_) {
            set_object_detected(true);
          } else if (distance >= object_clear_distance_) {
            set_object_detected(false);
          }
        });

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

      // Return the detection state to clear if SRF08 messages stop arriving.
      sensor_timeout_timer_ = create_wall_timer(
        std::chrono::milliseconds(sensor_check_period_ms_),
        [this]() {
          const auto now = std::chrono::steady_clock::now();
          const auto timeout = std::chrono::duration<double>(sensor_timeout_seconds_);
          if (object_detected_ &&
            (!received_sensor_message_ || now - last_sensor_message_time_ > timeout))
          {
            set_object_detected(false);
          }
        });

      // Return both servos home after the configured detection hold duration.
      hold_timer_ = create_wall_timer(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::duration<double>(hold_duration_seconds_)),
        [this]() {
          hold_timer_->cancel();
          RCLCPP_INFO(
            get_logger(), "Hold complete; returning both servos to %.1f degrees",
            home_angle_);
          set_servo_angle(servo1_pin_, home_angle_);
          set_servo_angle(servo2_pin_, home_angle_);
        });
      hold_timer_->cancel();

      RCLCPP_INFO(
        get_logger(),
        "DS3225 GPIO controller ready on BCM GPIO %d and %d; each detection on %s moves "
        "both servos to %.1f degrees for %.1f seconds",
        servo1_pin_, servo2_pin_, range_topic_.c_str(),
        object_detect_angle_, hold_duration_seconds_);
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
      !std::isfinite(home_angle_) || !std::isfinite(object_detect_angle_) ||
      min_angle_ < 180.0 || max_angle_ > 360.0 || min_angle_ >= max_angle_ ||
      home_angle_ < min_angle_ || home_angle_ > max_angle_ ||
      object_detect_angle_ < min_angle_ || object_detect_angle_ > max_angle_)
    {
      throw std::invalid_argument(
              "Servo angles must satisfy 180 <= min < max <= 360 and home/detection angles "
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
    if (!std::isfinite(object_detect_distance_) ||
      !std::isfinite(object_clear_distance_) ||
      object_detect_distance_ <= 0.0 ||
      object_detect_distance_ >= object_clear_distance_ ||
      !std::isfinite(sensor_timeout_seconds_) || sensor_timeout_seconds_ <= 0.0 ||
      sensor_check_period_ms_ <= 0 ||
      !std::isfinite(hold_duration_seconds_) || hold_duration_seconds_ <= 0.0)
    {
      throw std::invalid_argument(
              "Object distances must satisfy 0 < detect < clear, and sensor timeout and "
              "hold duration must be positive");
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

  // Apply rising-edge detection and start a timed movement for a new detection.
  void set_object_detected(bool detected)
  {
    if (detected == object_detected_) {
      return;
    }

    object_detected_ = detected;
    RCLCPP_INFO(
      get_logger(), "Object %s",
      object_detected_ ? "detected" : "cleared");
    if (object_detected_) {
      set_servo_angle(servo1_pin_, object_detect_angle_);
      set_servo_angle(servo2_pin_, object_detect_angle_);
      hold_timer_->reset();
    }
  }

  int gpio_chip_handle_{-1};
  bool servo1_claimed_{false};
  bool servo2_claimed_{false};
  int servo1_pin_{5};
  int servo2_pin_{6};
  double min_angle_{0.0};
  double max_angle_{180.0};
  double home_angle_{0.0};
  double object_detect_angle_{90.0};
  int min_pulse_width_us_{500};
  int max_pulse_width_us_{2500};
  int pwm_frequency_hz_{50};
  std::string range_topic_{"/scan/ultrasonic"};
  std::string servo1_command_topic_{"/ds3225/angle"};
  std::string servo2_command_topic_{"/ds3225/servo2/angle"};
  double object_detect_distance_{0.20};
  double object_clear_distance_{0.25};
  double sensor_timeout_seconds_{0.5};
  int sensor_check_period_ms_{100};
  double hold_duration_seconds_{2.0};
  bool object_detected_{false};
  bool received_sensor_message_{false};
  std::chrono::steady_clock::time_point last_sensor_message_time_{};
  rclcpp::Subscription<sensor_msgs::msg::Range>::SharedPtr range_subscription_;
  rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr servo1_subscription_;
  rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr servo2_subscription_;
  rclcpp::TimerBase::SharedPtr sensor_timeout_timer_;
  rclcpp::TimerBase::SharedPtr hold_timer_;
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
