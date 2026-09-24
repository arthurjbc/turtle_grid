#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "turtlesim/msg/pose.hpp"

using namespace std::chrono_literals;

const std::map<std::string, double> angle_dict = {
    {"right", -M_PI / 2},
    {"left", M_PI / 2},
    {"up", 0.0},
    {"down", M_PI}
};

#define ANGULAR_VEL 4.0
#define LINEAR_VEL 1.4
#define ERROR_MARGIN 0.0001
#define CAPPING_HORIZONTAL 10.9

#define STANDBY "stand-by"
#define ROTATING "rotating"
#define MOVING "moving"

class TurtleNode : public rclcpp::Node {
    public:
        TurtleNode() : Node("turtle"){
            RCLCPP_INFO(this->get_logger(), "turtle has been started");

            // Inscricao no topico de comandos
            cmd_sub = this->create_subscription<std_msgs::msg::String>("/cmd_turtle", 10, std::bind(&TurtleNode::command_callback, this, std::placeholders::_1));

            // Inscricao no topico de posicao da tartaruga para movimento de rotacao
            pose_sub = this->create_subscription<turtlesim::msg::Pose>("/turtle1/pose", 10, std::bind(&TurtleNode::pose_callback, this, std::placeholders::_1));

            // Publisher para mover a tartaruga no turtlesim
            vel_pub = this->create_publisher<geometry_msgs::msg::Twist>("/turtle1/cmd_vel", 10);

            // Timer de controle da maquina de estado (20hz = 0.05s por ciclo)
            control_timer = this->create_wall_timer(50ms, std::bind(&TurtleNode::cmd_control, this));
        }

    private:
        void pose_callback(const turtlesim::msg::Pose::SharedPtr msg){
            pose = msg;
        }

        void command_callback(const std_msgs::msg::String::SharedPtr msg){
            const std::string command = msg->data;

            if (state != STANDBY) {
                RCLCPP_WARN(this->get_logger(), "Em movimento, ignorando: %s", command.c_str());
                return;
            }

            if (pose == nullptr) {
                RCLCPP_WARN(this->get_logger(), "Ainda sem pose, ignorando: %s", command.c_str());
                return;
            }

            pending_command = command;
            target_theta = std::atan2(std::sin(pose->theta + angle_dict.at(command)), std::cos(pose->theta + angle_dict.at(command)));
            state = ROTATING;
            RCLCPP_INFO(this->get_logger(), "Comando recebido: %s", command.c_str());
        }

        void cmd_control(){
            if (state == STANDBY) {
                return;
            } else if (state == ROTATING) {
                do_rotation();
            } else if (state == MOVING) {
                do_movement();
            }
        }

        void do_rotation(){
            double error = std::atan2(std::sin(target_theta - pose->theta), std::cos(target_theta - pose->theta));

            if (std::abs(error) < ERROR_MARGIN) {
                stop_turtle();
                start_x = pose->x;
                start_y = pose->y;
                state = MOVING;
                if (capping()) {
                    RCLCPP_WARN(this->get_logger(), "Capping em (%d, %d)",
                                static_cast<int>(pose->x - 5.544445),
                                static_cast<int>(pose->y - 5.544445)
                            );
                    stop_turtle();
                    pending_command.clear();
                    state = STANDBY;
                    return;
                }
                return;
            }

            auto twist = geometry_msgs::msg::Twist();
            twist.angular.z = std::max(-ANGULAR_VEL, std::min(ANGULAR_VEL, 4.0 * error));
            vel_pub->publish(twist);
        }

        bool capping(){
            double next_x = pose->x + std::cos(pose->theta);
            double next_y = pose->y + std::sin(pose->theta);

            return !(0.2 <= next_x && next_x <= CAPPING_HORIZONTAL && 0.2 <= next_y && next_y <= CAPPING_HORIZONTAL);
        }

        void do_movement(){

            const double traveled = std::hypot(pose->x - start_x, pose->y - start_y);

            if (traveled >= 1.0) {
                stop_turtle();
                finish_command();
                return;
            }

            auto twist = geometry_msgs::msg::Twist();
            twist.linear.x = LINEAR_VEL;
            vel_pub->publish(twist);
        }

        void finish_command(){
            x=(int) round(pose->x - 5.544445);
            y=(int) round(pose->y - 5.544445);
            RCLCPP_INFO(this->get_logger(), "Posicao atual: (%d, %d); Posição real: (%f, %f)", x, y, (pose->x), (pose->y));
            RCLCPP_INFO(this->get_logger(), "Theta atual: (%f), command = %s", pose->theta, pending_command.c_str());

            pending_command.clear();
            state = STANDBY;
        }

        void stop_turtle(){
            auto stop = geometry_msgs::msg::Twist();
            vel_pub->publish(stop);
        }

        rclcpp::Subscription<std_msgs::msg::String>::SharedPtr cmd_sub;
        rclcpp::Subscription<turtlesim::msg::Pose>::SharedPtr pose_sub;
        rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr vel_pub;
        rclcpp::TimerBase::SharedPtr control_timer;

        int x = 0;
        int y = 0;

        std::string state = STANDBY;
        double target_theta = 0.0;
        std::string pending_command;
        double start_x = 0.0;
        double start_y = 0.0;

        turtlesim::msg::Pose::SharedPtr pose = nullptr;
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<TurtleNode>();

    rclcpp::spin(node);

    rclcpp::shutdown();
    return 0;
}
