#include <chrono>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

const std::vector<std::string> COMMANDS = {"right", "left", "up", "down"};

class SendMsgNode : public rclcpp::Node{
    public:
        SendMsgNode() : Node("send_msg"), gen_(std::random_device{}()), dist_(0, COMMANDS.size() - 1){
            // Tipo de mensagem, topico e queue size
            cmd_pub = this->create_publisher<std_msgs::msg::String>("/cmd_turtle", 10);

            timer = this->create_wall_timer(4s, std::bind(&SendMsgNode::send_command, this));

            RCLCPP_INFO(this->get_logger(), "send_msg has been started");
        }

    private:
        void send_command(){
            auto msg = std_msgs::msg::String();
            msg.data = COMMANDS[dist_(gen_)];

            cmd_pub->publish(msg);
            RCLCPP_INFO(this->get_logger(), "Publicado: %s", msg.data.c_str());
        }

        rclcpp::Publisher<std_msgs::msg::String>::SharedPtr cmd_pub;
        rclcpp::TimerBase::SharedPtr timer;

        std::mt19937 gen_;
        std::uniform_int_distribution<std::size_t> dist_;
};

int main(int argc, char **argv){
    rclcpp::init(argc, argv);
    auto node = std::make_shared<SendMsgNode>();

    rclcpp::spin(node);

    rclcpp::shutdown();
    return 0;
}
