# TechOWeenBot
Skeleton robot that utilizes ros framework, an ultrasonic sensor, and servo motors to operate a skeleton to scare people who come to our spooky IEEE booth

## DS3225 servos and Arduino jaw

`ds3225.launch.py` starts the existing Raspberry Pi GPIO controller for the two
skeleton servos. `ds3225_serial.launch.py` starts the Arduino serial bridge at
`/dev/ttyACM0` and 115200 baud. On the Arduino, the existing servo channels
remain on digital pins 5 and 6; the jaw is an additional channel 3 on digital
pin 7. The speaker node opens the jaw while audio plays and closes it when
playback ends.

### Arduino wiring

- Connect the jaw servo's signal wire to Arduino digital pin 7.
- Keep the existing Arduino servo signals on digital pins 5 and 6.
- Connect servo power and ground appropriately, with a shared ground between
  the Arduino and servo power supply.
