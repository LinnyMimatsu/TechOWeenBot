
#ifndef FS90R_HARDWARE__FS90R_SERVO_HPP_
#define FS90R_HARDWARE__FS90R_SERVO_HPP_

class FS90RServo
{
public:
  explicit FS90RServo(
    int bcm_gpio_pin,
    int neutral_us = 1500,
    int min_us = 1000,
    int max_us = 2000);

  ~FS90RServo();

  FS90RServo(const FS90RServo &) = delete;
  FS90RServo & operator=(const FS90RServo &) = delete;

  void set_normalized_speed(double speed);
  void stop();

private:
  void set_pulse_width(int pulse_width_us);

  int pin_;
  int neutral_us_;
  int min_us_;
  int max_us_;

  int gpiochip_handle_{-1};
  int current_pulse_us_{-1};
};

#endif