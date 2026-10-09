
#include "fs90r_hardware/fs90r_servo.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <thread>

#include <lgpio.h>

FS90RServo::FS90RServo(
  int bcm_gpio_pin, int neutral_us, int min_us, int max_us)
: pin_(bcm_gpio_pin),
  neutral_us_(neutral_us),
  min_us_(min_us),
  max_us_(max_us)
{
  if (!(min_us_ < neutral_us_ && neutral_us_ < max_us_) ||
      min_us_ < 500 || max_us_ > 2500) {
    throw std::invalid_argument(
      "Invalid FS90R pulse-width configuration");
  }

  // Raspberry Pi 4 GPIO controller.
  gpiochip_handle_ = lgGpiochipOpen(0);

  if (gpiochip_handle_ < 0) {
    throw std::runtime_error(
      "lgpio could not open GPIO chip 0");
  }

  const int claim_result =
    lgGpioClaimOutput(gpiochip_handle_, 0, pin_, 0);

  if (claim_result < 0) {
    lgGpiochipClose(gpiochip_handle_);
    gpiochip_handle_ = -1;
    throw std::runtime_error(
      "lgpio could not claim the servo GPIO as an output");
  }

  // Start with a neutral pulse immediately.
  set_pulse_width(neutral_us_);
}

FS90RServo::~FS90RServo()
{
  if (gpiochip_handle_ >= 0) {
    // Keep sending neutral pulses before shutting down.
    try {
      set_pulse_width(neutral_us_);
      std::this_thread::sleep_for(
        std::chrono::milliseconds(100));

      // Stop pulse transmission after the neutral interval.
      lgTxServo(
        gpiochip_handle_, pin_, 0, 50, 0, 0);

      lgGpioWrite(gpiochip_handle_, pin_, 0);
      lgGpioFree(gpiochip_handle_, pin_);
      lgGpiochipClose(gpiochip_handle_);
    } catch (...) {
      // Destructors must not throw.
    }

    gpiochip_handle_ = -1;
  }
}

void FS90RServo::set_pulse_width(int pulse_width_us)
{
  if (gpiochip_handle_ < 0) {
    throw std::runtime_error("lgpio is not initialized");
  }

  // Avoid repeatedly queuing identical commands.
  if (pulse_width_us == current_pulse_us_) {
    return;
  }

  const int result =
    lgTxServo(
      gpiochip_handle_,
      pin_,
      pulse_width_us,
      50,   // Servo frequency: 50 Hz
      0,    // Pulse offset
      0);   // Repeat indefinitely

  if (result < 0) {
    throw std::runtime_error(
      "lgpio failed to transmit servo pulses");
  }

  current_pulse_us_ = pulse_width_us;
}

void FS90RServo::set_normalized_speed(double speed)
{
  speed = std::clamp(speed, -1.0, 1.0);

  int pulse = neutral_us_;

  if (speed > 0.0) {
    pulse += static_cast<int>(
      speed * (max_us_ - neutral_us_));
  } else if (speed < 0.0) {
    pulse += static_cast<int>(
      speed * (neutral_us_ - min_us_));
  }

  set_pulse_width(pulse);
}

void FS90RServo::stop()
{
  set_pulse_width(neutral_us_);
}