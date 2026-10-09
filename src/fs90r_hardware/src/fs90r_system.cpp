
#include "fs90r_hardware/fs90r_system.hpp"

#include <algorithm>
#include <exception>
#include <string>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "pluginlib/class_list_macros.hpp"
#include "rclcpp/rclcpp.hpp"

namespace fs90r_hardware
{

hardware_interface::CallbackReturn FS90RSystem::on_init(
  const hardware_interface::HardwareInfo & info)
{
  if (hardware_interface::SystemInterface::on_init(info) !=
      hardware_interface::CallbackReturn::SUCCESS) {
    return hardware_interface::CallbackReturn::ERROR;
  }

  if (info_.joints.size() != 1) {
    RCLCPP_ERROR(
      rclcpp::get_logger("FS90RSystem"),
      "This minimal example supports exactly one FS90R joint.");
    return hardware_interface::CallbackReturn::ERROR;
  }

  try {
    const auto pin_it = info_.hardware_parameters.find("gpio_pin");
    if (pin_it == info_.hardware_parameters.end()) {
      RCLCPP_ERROR(
        rclcpp::get_logger("FS90RSystem"),
        "Missing hardware parameter 'gpio_pin'. Use a BCM GPIO number.");
      return hardware_interface::CallbackReturn::ERROR;
    }

    const int pin = std::stoi(pin_it->second);
    const int neutral_us = info_.hardware_parameters.count("neutral_us")
      ? std::stoi(info_.hardware_parameters.at("neutral_us")) : 1500;
    const int min_us = info_.hardware_parameters.count("min_us")
      ? std::stoi(info_.hardware_parameters.at("min_us")) : 1000;
    const int max_us = info_.hardware_parameters.count("max_us")
      ? std::stoi(info_.hardware_parameters.at("max_us")) : 2000;

    if (info_.hardware_parameters.count("max_velocity_rad_s")) {
      max_velocity_rad_s_ =
        std::stod(info_.hardware_parameters.at("max_velocity_rad_s"));
    }

    if (max_velocity_rad_s_ <= 0.0) {
      RCLCPP_ERROR(
        rclcpp::get_logger("FS90RSystem"),
        "'max_velocity_rad_s' must be positive.");
      return hardware_interface::CallbackReturn::ERROR;
    }

    servo_ = std::make_unique<FS90RServo>(
      pin, neutral_us, min_us, max_us);
  } catch (const std::exception & e) {
    RCLCPP_ERROR(
      rclcpp::get_logger("FS90RSystem"),
      "Failed to initialize FS90R: %s", e.what());
    return hardware_interface::CallbackReturn::ERROR;
  }

  command_velocity_ = 0.0;
  state_velocity_ = 0.0;
  return hardware_interface::CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface>
FS90RSystem::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> interfaces;
  interfaces.emplace_back(
    info_.joints[0].name,
    hardware_interface::HW_IF_VELOCITY,
    &state_velocity_);
  return interfaces;
}

std::vector<hardware_interface::CommandInterface>
FS90RSystem::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> interfaces;
  interfaces.emplace_back(
    info_.joints[0].name,
    hardware_interface::HW_IF_VELOCITY,
    &command_velocity_);
  return interfaces;
}

hardware_interface::CallbackReturn FS90RSystem::on_configure(
  const rclcpp_lifecycle::State &)
{
  command_velocity_ = 0.0;
  state_velocity_ = 0.0;

  if (servo_) {
    servo_->stop();
  }

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn FS90RSystem::on_activate(
  const rclcpp_lifecycle::State &)
{
  command_velocity_ = 0.0;
  state_velocity_ = 0.0;

  if (servo_) {
    servo_->stop();
  }

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn FS90RSystem::on_deactivate(
  const rclcpp_lifecycle::State &)
{
  command_velocity_ = 0.0;
  state_velocity_ = 0.0;

  if (servo_) {
    servo_->stop();
  }

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::return_type FS90RSystem::read(
  const rclcpp::Time &, const rclcpp::Duration &)
{
  // This is the last accepted command, not measured physical speed.
  state_velocity_ = command_velocity_;
  return hardware_interface::return_type::OK;
}

hardware_interface::return_type FS90RSystem::write(
  const rclcpp::Time &,
  const rclcpp::Duration &)
{
  if (!servo_) {
    return hardware_interface::return_type::ERROR;
  }

  const double velocity = std::clamp(
    command_velocity_,
    -max_velocity_rad_s_,
    max_velocity_rad_s_);

  const double normalized_speed =
    velocity / max_velocity_rad_s_;

  servo_->set_normalized_speed(normalized_speed);
  state_velocity_ = velocity;

  return hardware_interface::return_type::OK;
}

}  // namespace fs90r_hardware

PLUGINLIB_EXPORT_CLASS(
  fs90r_hardware::FS90RSystem, hardware_interface::SystemInterface)