#include "laser_geometry/laser_geometry.h"
#include "tf/tf.h"
#include <ros/ros.h>
#include <tf/transform_broadcaster.h>

#include <boost/asio.hpp>
#include <boost/timer.hpp>
#include <boost/bind.hpp>

#include <pcl_ros/transforms.h>
#include<pcl_ros/point_cloud.h>
#include <fstream>

std::fstream fout;
ros::Subscriber sub;
void readCallback(const sensor_msgs::PointCloud2::ConstPtr& point_msg){

    sensor_msgs::PointCloud2 pointcloud_tmp=*point_msg;

    int x_idx = pcl::getFieldIndex(pointcloud_tmp,"x");
    int y_idx = pcl::getFieldIndex(pointcloud_tmp,"y");
    int z_idx = pcl::getFieldIndex(pointcloud_tmp,"z");

    int dist_idx = pcl::getFieldIndex(pointcloud_tmp,"distance");

    Eigen::Array4i xyz_offset (pointcloud_tmp.fields[x_idx].offset,pointcloud_tmp.fields[y_idx].offset,pointcloud_tmp.fields[z_idx].offset,0);
    for (size_t i = 0;i<pointcloud_tmp.width * pointcloud_tmp.height;++i)
    {
        Eigen::Vector4f pt(*(float *)&pointcloud_tmp.data[xyz_offset[0]],*(float*)&pointcloud_tmp.data[xyz_offset[1]],*(float*)&pointcloud_tmp.data[xyz_offset[2]],1);
        int distance_ptr_offset = i * pointcloud_tmp.point_step + pointcloud_tmp.fields[dist_idx].offset;
        float * distance_ptr = (dist_idx<0?NULL:(float*)(&pointcloud_tmp.data[distance_ptr_offset]));

            if (distance_ptr == NULL || !std::isfinite(*distance_ptr))   //Invalid point
            {
                pt = pt;
            }else{//max range point
                pt[0] = *distance_ptr; //Replace x with the x value saved in distance
            }

        //std::cout<<i<<" : "<<pt[0]<<" "<<pt[1]<<" "<<pt[2]<<" "<<pt[3]<<std::endl;
        std::cout<< pt[0]<<" "<<pt[1]<<" "<<pt[2]<<std::endl;
        xyz_offset += pointcloud_tmp.point_step;



    }



}

int main(int argc, char **argv){
    ros::init(argc, argv, "sum_pointcloud");
    ros::NodeHandle n;
    //ROS_INFO("Start get pointcloud.");

    fout.open("/home/lixin/pointcloudxyz");

    sub = n.subscribe("laser_cloud_surround",1,readCallback);





    ros::spin();
    fout.close();

    return 0;
}

