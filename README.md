# TechOWeenBot
Skeleton robot that utilizes ros framework, an ultrasonic sensor, and servo motors to operate a skeleton to scare people who come to our spooky IEEE booth

## DS3225 servos with Raspberry Pi GPIO

The default DS3225 controller generates servo PWM directly from Raspberry Pi
GPIO using `lgpio`. DS3225 servos accept position commands but do not report
their actual shaft position. The node uses BCM GPIO 5 and 6 by default and
subscribes to the SRF08 publisher's `/scan/ultrasonic` topic.

### Raspberry Pi wiring

- Connect servo 1's signal wire to Raspberry Pi BCM GPIO 5 (physical pin 29).
- Connect servo 2's signal wire to Raspberry Pi BCM GPIO 6 (physical pin 31).
