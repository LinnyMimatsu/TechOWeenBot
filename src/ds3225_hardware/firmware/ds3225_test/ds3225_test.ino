#include <Servo.h>

constexpr uint8_t SERVO_1_SIGNAL_PIN = 5;
constexpr uint8_t SERVO_2_SIGNAL_PIN = 6;
constexpr int MIN_ANGLE = 0;
constexpr int MAX_ANGLE = 180;

Servo servo1;
Servo servo2;
String command;

// Initialize serial communication and attach both servos at their home positions.
void setup()
{
  Serial.begin(115200);
  servo1.attach(SERVO_1_SIGNAL_PIN, 500, 2500);
  servo2.attach(SERVO_2_SIGNAL_PIN, 500, 2500);
  servo1.write(0);
  servo2.write(0);
  Serial.println("READY");
}

// Parse serial commands, move the selected servo, and acknowledge each result.
void loop()
{
  while (Serial.available() > 0) {
    const char character = static_cast<char>(Serial.read());
    if (character == '\n') {
      command.trim();
      if (command == "?") {
        Serial.println("READY");
        command = "";
        continue;
      }

      const int separator = command.indexOf(':');
      const String servo_id = separator >= 0 ? command.substring(0, separator) : "";
      const String angle_text = separator >= 0 ? command.substring(separator + 1) : "";
      const int angle = angle_text.toInt();
      const bool valid =
        separator == 1 &&
        (servo_id == "1" || servo_id == "2") &&
        angle_text.length() > 0 &&
        String(angle) == angle_text &&
        angle >= MIN_ANGLE &&
        angle <= MAX_ANGLE;

      if (valid) {
        if (servo_id == "1") {
          servo1.write(angle);
        } else {
          servo2.write(angle);
        }
        Serial.print("OK:");
        Serial.print(servo_id);
        Serial.print(':');
        Serial.println(angle);
      } else {
        Serial.println("ERR");
      }
      command = "";
    } else if (command.length() < 8) {
      command += character;
    } else {
      command = "";
      Serial.println("ERR");
    }
  }
}
