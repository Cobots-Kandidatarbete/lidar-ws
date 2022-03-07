#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"
#include "librealsense2/rs.hpp" // Include RealSense Cross Platform API
#include "pcl/point_cloud.h"
#include "pcl/point_types.h"
#include "pcl_conversions/pcl_conversions.h"
#include "lidar_cpp/srv/lidar_service.hpp"

void take_picture(const std::shared_ptr<lidar_cpp::srv::LidarService::Response> response)
{
    rs2::pipeline pipe;
    rs2::pointcloud pc;
    rs2::points points;
    pipe.start();
    auto frames = pipe.wait_for_frames();
    auto depth = frames.get_depth_frame();
    points = pc.calculate(depth);
    pcl::PCLPointCloud2 pcl_points;
    pcl::toPCLPointCloud2(*points_to_pcl(points), pcl_points);

    sensor_msgs::msg::PointCloud2 message;
    pcl_conversions::fromPCL(pcl_points, message);
    message.header.frame_id = "world";
    response->pcl_response = message;
    pipe.stop();
}

using pcl_ptr = pcl::PointCloud<pcl::PointXYZ>::Ptr;
pcl_ptr points_to_pcl(const rs2::points &points)
{
    pcl_ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);

    auto sp = points.get_profile().as<rs2::video_stream_profile>();
    cloud->width = sp.width();
    cloud->height = sp.height();
    cloud->is_dense = false;
    cloud->points.resize(points.size());
    auto ptr = points.get_vertices();
    for (auto &p : cloud->points)
    {
        p.x = ptr->x;
        p.y = ptr->y;
        p.z = ptr->z;
        ptr++;
    }

    return cloud;
}

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    std::shared_ptr<rclcpp::Node> node = rclcpp::Node::make_shared("lidar_service_server");
    rclcpp::Service<lidar_cpp::srv::LidarService>::SharedPtr service =
        node->create_service<lidar_cpp::srv::LidarService>("lidar_service", &take_picture);
    rclcpp::spin(node);
    rclcpp::shutdown();
}