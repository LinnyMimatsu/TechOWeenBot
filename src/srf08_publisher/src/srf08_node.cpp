#include <chrono>
#include <memory>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/range.hpp"

using namespace std::chrono_literals;

class SRF08Publisher : public rclcpp::Node {
public:
    SRF08Publisher() : Node("srf08_publisher"), i2c_device_("/dev/i2c-1"), i2c_address_(0x73) {
        
        //initialize i2c connection
        if (!init_i2c()) {
            RCLCPP_ERROR(this->get_logger(), "Initialization failed. Exiting.");
            throw std::runtime_error("I2C initialization failure");
        }

        //create ros2 publisher
        publisher_ = this->create_publisher<sensor_msgs::msg::Range>("ultrasonic/range", 10);

        //timer to poll sensor at 10 hz (every 100ms)
        timer_ = this->create_wall_timer(100ms, std::bind(&SRF08Publisher::timer_callback, this));
        
        RCLCPP_INFO(this->get_logger(), "SRF08 C++ ROS 2 Node initialized at address 0x73.");
    }

    ~SRF08Publisher() {
        if (i2c_fd_ >= 0) {
            close(i2c_fd_);
        }
    }

    //post if the sensor isnt detected or has issues
private:
    bool init_i2c() {
        i2c_fd_ = open(i2c_device_.c_str(), O_RDWR);
        if (i2c_fd_ < 0) {
            RCLCPP_ERROR(this->get_logger(), "Failed to open I2C bus: %s", i2c_device_.c_str());
            return false;
        }

        if (ioctl(i2c_fd_, I2C_SLAVE, i2c_address_) < 0) {
            RCLCPP_ERROR(this->get_logger(), "Failed to acquire bus access to address 0x%02X", i2c_address_);
            close(i2c_fd_);
            return false;
        }
        return true;
    }

    void timer_callback() {

        //registers & commands
        const uint8_t COMMAND_REG = 0x00;
        const uint8_t PING_IN_CM = 0x51;
        const uint8_t RANGE_HIGH_BYTE = 0x02;

        //trigger the ping command (result in cm)
        uint8_t write_buf[2] = {COMMAND_REG, PING_IN_CM};
        if (write(i2c_fd_, write_buf, 2) != 2) {
            RCLCPP_ERROR(this->get_logger(), "Failed to write ping command to sensor.");
            return;
        }

        //wait for acoustic echo to complete (max 65ms required)
        std::this_thread::sleep_for(70ms);

        //request reading starting from register 0x02
        if (write(i2c_fd_, &RANGE_HIGH_BYTE, 1) != 1) {
            RCLCPP_ERROR(this->get_logger(), "Failed to set read register pointer.");
            return;
        }

        //read distance high and low bytes
        uint8_t read_buf[2] = {0};
        if (read(i2c_fd_, read_buf, 2) != 2) {
            RCLCPP_ERROR(this->get_logger(), "Failed to read range data from sensor.");
            return;
        }

        uint16_t distance_cm = (read_buf[0] << 8) | read_buf[1];
        double distance_meters = distance_cm / 100.0;

        RCLCPP_INFO(this->get_logger(), "Distance: %d cm (%.2f m)", distance_cm, distance_meters);
        
        //construct and publish standard ros2 range message
        auto msg = sensor_msgs::msg::Range();
        msg.header.stamp = this->get_clock()->now();
        msg.header.frame_id = "ultrasonic_sensor_link";
        msg.radiation_type = sensor_msgs::msg::Range::ULTRASOUND;
        msg.field_of_view = 0.96; // ~55 degrees in radians
        msg.min_range = 0.03; // 3 cm (m)
        msg.max_range = 6.0; //meters
        msg.range = static_cast<float>(distance_meters);

        publisher_->publish(msg);
        RCLCPP_DEBUG(this->get_logger(), "Published Range: %.2f m", distance_meters);
    }

    std::string i2c_device_;
    int i2c_address_;
    int i2c_fd_ = -1;
    rclcpp::Publisher<sensor_msgs::msg::Range>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    try {
        rclcpp::spin(std::make_shared<SRF08Publisher>());
    } catch (const std::exception& e) {
        RCLCPP_FATAL(rclcpp::get_logger("rclcpp"), "Node terminated with exception: %s", e.what());
    }
    rclcpp::shutdown();
    return 0;
}
