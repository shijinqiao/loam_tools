#include "laser_geometry/laser_geometry.h"
#include "tf/tf.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/timer.hpp>

#include <pcl_ros/transforms.h>
#include<pcl_ros/point_cloud.h>
#include <fstream>

#include <vector>
ros::Subscriber sub;
ros::Publisher pub;


boost::asio::io_service io;
boost::asio::serial_port sp(io,"/dev/ttyUSB0");
double w=0;
int last_sum;
double last_endtime;

//for debug
std::fstream fout;
std::fstream foutlaser;

double time_before,time_now;
double sum_before,sum_now;

static double scan_time_old=0;
double last_avg_v(7200);

//vector<double> time_stamp;
//vector<double> sum_vector;
void handle_read(char * buf,boost::system::error_code ec, std::size_t bytes_transferred){

}


void lCallback(const sensor_msgs::LaserScan::ConstPtr& scan_msg)
{


    laser_geometry::LaserProjection p;

    sensor_msgs::PointCloud2 pointcloud_tmp;

    double scan_time,scan_diff,point_scan_diff,point_cloud_time;

    scan_time= scan_msg->header.stamp.toSec();
    scan_diff=scan_time-scan_time_old;
    scan_time_old=scan_time;
    p.projectLaser(*scan_msg,pointcloud_tmp,-1.0,3);//1.0 - 2.0  or -1.0

    point_cloud_time = pointcloud_tmp.header.stamp.toSec();

    point_scan_diff=point_cloud_time-scan_time;
    pointcloud_tmp.header.frame_id="/camera";

    Eigen::Matrix4f Tm= Eigen::Matrix4f::Identity();
    Eigen::Matrix4f Tm1 = Eigen::Matrix4f::Identity();
    Eigen::Matrix4f Tm2 = Eigen::Matrix4f::Identity();
    Eigen::Matrix4f Tm3 = Eigen::Matrix4f::Identity();

    bool isok(false);
    char bufread[10];
    int a(0),b(0),c(0),sum;
    sum = 0;
    int times(0);
    double serial_time;

    while(isok==false)//ver 1.0--read
    {
        memset(bufread,0,10);
        if((sp.is_open()))
        {
            //ROS_INFO("2-is open---");
            boost::asio::read(sp,boost::asio::buffer(bufread));
        }else{
            ROS_INFO("sp error");
        }
        serial_time = ros::Time::now().toSec();

        for(int i =0 ;i< 10;i++)
        {
            bufread[i] =  bufread[i] & 0xff ;

        }

        for(int i = 0;i < 10;i++)
        {


            if((bufread[i] & 0xff) == 0x55)
            {

                a = 0xff & bufread[i+1];
                b = 0xff & bufread[i+2];
                c = 0xff & bufread[i+3];

                if( (b > -1) && (c == ((a+b) % 256)) )
                {

                    sum = a * 255 + b;

                    if(sum<7201 )
                    {
                        //if(i<1)
                            //serial_time-=0.01;
                        if( i<6)
                            serial_time-=0.01;
                        isok = true;
                        sum_now = sum;
                        //std::cout <<i<<std::endl;
                        break;
                    }

                }
            }

        }
        times++;
        if(times > 1)
        {
            std::cout << "get sum false!"<<std::endl;
            return;

        }
    }
    time_now = serial_time;

    double endtime = serial_time - scan_time;pointcloud_tmp.header.stamp.toSec();
    //std::cout << endtime <<std::endl;


    double avg_v;


    if(sum_now < sum_before)
        sum_now += 7200;
    avg_v = last_avg_v;//(((sum_before - sum_now)/(time_before - time_now)));


    sum = sum-endtime * avg_v ;
    if(sum < 0)
        sum +=7200;
    if(sum>7200) sum-=7200;
    time_before = time_now;
    sum_before = sum;


    //w is the angle of lidar
    w =(sum /20 );//-180 ;
    last_endtime = endtime;
    last_sum = sum;
    float theta=3.1415926 * w / 180;
    //std::cout <<"last_avg_v:"<<last_avg_v<<"avg_v:"<<avg_v<<"endtime*avg_v:"<<endtime*avg_v<<"sum:"<<sum<<"w:"<<w<<std::endl;
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////atan2////////////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    ///
    ///
    ///
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
    transform = Tm;

    int x_idx = pcl::getFieldIndex(pointcloud_tmp,"x");
    int y_idx = pcl::getFieldIndex(pointcloud_tmp,"y");
    int z_idx = pcl::getFieldIndex(pointcloud_tmp,"z");


    //check if distance is available
    int dist_idx = pcl::getFieldIndex(pointcloud_tmp,"distance");
    //xyz_offset
    Eigen::Array4i xyz_offset (pointcloud_tmp.fields[x_idx].offset,pointcloud_tmp.fields[y_idx].offset,pointcloud_tmp.fields[z_idx].offset,0);
    double ntheta;
    int static ok=0;
    //std::cout <<"-------------------------------"<<std::endl;
    for (size_t i = 0;i<pointcloud_tmp.width * pointcloud_tmp.height;++i)
    {

        Eigen::Vector4f pt(*(float *)&pointcloud_tmp.data[xyz_offset[0]],*(float*)&pointcloud_tmp.data[xyz_offset[1]],*(float*)&pointcloud_tmp.data[xyz_offset[2]],1);
        Eigen::Vector4f pt_out;


        ntheta = theta+(0.025/8*avg_v*3.1415926/180/20)+(((135+(double)(atan2(pt[1], pt[0])*180/3.14159265))/270)*avg_v*0.025/20*3/4*3.1415926/180)*8/10;

        //std::cout<<((135+(double)(atan2(pt[1], pt[0])*180/3.14159265))/270*avg_v*0.025/20*3/4)<<std::endl;//<<"     " <<((135+(atan2(pt[1], pt[1])*180/3.14159265))/270*0.025*avg_v/20*3/4)/270*0.025<<std::endl;
                //ntheta = theta - avg_v*(0.025/8+(pointcloud_tmp.width-i)/pointcloud_tmp.width*0.025*3/4);
        //std::cout << atan2(pt[1], pt[0])*180/3.14159265<<std::endl;
        //std::cout<< theta<<"   "<<ntheta<<std::endl;
        //if(ntheta<0) ntheta+=6.2831852;

        //ntheta = theta;


        transform(0,0)=1;
        transform(1,1)=cos(ntheta);
        transform(1,2)=-sin(ntheta);
        transform(2,1)=sin(ntheta);
        transform(2,2)=cos(ntheta);
        transform(2,3)=cos(ntheta)* -0.00 ;//* -0.001;
        transform(1,3)=-sin(ntheta)*  -0.00 ;//* -0.001;
        transform(3,3) = 1;





        bool max_range_point = false;
        int distance_ptr_offset = i * pointcloud_tmp.point_step + pointcloud_tmp.fields[dist_idx].offset;
        float * distance_ptr = (dist_idx<0?NULL:(float*)(&pointcloud_tmp.data[distance_ptr_offset]));
        if(!std::isfinite(pt[0]) || !std::isfinite(pt[1]) || !std::isfinite(pt[2]))
        {
            if (distance_ptr == NULL || !std::isfinite(*distance_ptr))   //Invalid point
            {
                pt_out = pt;
            }else{//max range point
                pt[0] = *distance_ptr; //Replace x with the x value saved in distance
                pt_out = transform * pt;//
                max_range_point = true;
            }
        }else{
            pt_out = transform * pt;
        }

        if(max_range_point)
        {
            //Save x. value in distance again
            *(float*)(&pointcloud_tmp.data[distance_ptr_offset]) = pt_out[0];
            pt_out[0] = std::numeric_limits<float> ::quiet_NaN();
        }
        pt_out[2] = - pt_out[2];
        memcpy(&pointcloud_tmp.data[xyz_offset[2]],&pt_out[0], sizeof(float));
        memcpy(&(pointcloud_tmp.data[xyz_offset[0]]),&pt_out[1], sizeof(float));
        memcpy(&pointcloud_tmp.data[xyz_offset[1]],&pt_out[2], sizeof(float));

        xyz_offset += pointcloud_tmp.point_step;


    }

    ok++;


    pub.publish(pointcloud_tmp);

}

int main(int argc,char **argv)
{

    ROS_INFO("Now,start the node succeful!");



    fout.open("/home/lixin/data.csv");
    foutlaser.open("/home/lixin/datalaser.csv");
    std::cout <<"....................1....................."<<std::endl;



    sp.set_option(boost::asio::serial_port::baud_rate(115200));
    sp.set_option(boost::asio::serial_port::flow_control());
    sp.set_option(boost::asio::serial_port::parity());
    sp.set_option(boost::asio::serial_port::stop_bits());
    sp.set_option(boost::asio::serial_port::character_size(8));

    ////////begin
    boost::asio::write(sp,boost::asio::buffer("\xAA",1));
    boost::asio::write(sp,boost::asio::buffer("\x55",1));
    boost::asio::write(sp,boost::asio::buffer("\xFA",1));
    boost::asio::write(sp,boost::asio::buffer("\x01",1));
    boost::asio::write(sp,boost::asio::buffer("\xFB",1));

    ROS_INFO("step2111");
    //sleep(2);
    ros::init(argc,argv,"laser_geometry_node");
    ros::NodeHandle n;
    pub=n.advertise<sensor_msgs::PointCloud2>("sync_scan_cloud_filtered",1);
    sub=n.subscribe("first",1,lCallback);





    while(ros::ok()){

        ros::spinOnce();
        char buf[5];
        boost::asio::io_service iosev;
        boost::asio::serial_port sp_tmp(iosev,"/dev/ttyUSB0");
        memset(buf,0,5);
        boost::asio::async_read(sp_tmp,boost::asio::buffer(buf),boost::bind(handle_read,buf,_1,_2));
        boost::asio::deadline_timer timer(iosev);

        timer.expires_from_now(boost::posix_time::microseconds(100));
        timer.async_wait(boost::bind(&boost::asio::serial_port::cancel,boost::ref(sp_tmp)));


        iosev.run();
        sp_tmp.close();
        for (int kk = 1;kk<1000;kk++);




    }
    std::cout <<"....................2....................."<<std::endl;
    //std::cout <<"....................2....................."<<std::endl;
    ////////stopAA550A020C
    boost::asio::write(sp,boost::asio::buffer("\xAA",1));
    boost::asio::write(sp,boost::asio::buffer("\x55",1));
    boost::asio::write(sp,boost::asio::buffer("\xFA",1));
    boost::asio::write(sp,boost::asio::buffer("\x02",1));
    boost::asio::write(sp,boost::asio::buffer("\xFc",1));
    sleep(2);
    std::cout <<"....................2....................."<<std::endl;
    //std::cout <<"....................2....................."<<std::endl;
    ////////stopAA550A020C
    boost::asio::write(sp,boost::asio::buffer("\xAA",1));
    boost::asio::write(sp,boost::asio::buffer("\x55",1));
    boost::asio::write(sp,boost::asio::buffer("\xFA",1));
    boost::asio::write(sp,boost::asio::buffer("\x02",1));
    boost::asio::write(sp,boost::asio::buffer("\xFc",1));
    sleep(4);
    std::cout <<"....................2....................."<<std::endl;
    //std::cout <<"....................2....................."<<std::endl;
    ////////stopAA550A020C
    boost::asio::write(sp,boost::asio::buffer("\xAA",1));
    boost::asio::write(sp,boost::asio::buffer("\x55",1));
    boost::asio::write(sp,boost::asio::buffer("\xFA",1));
    boost::asio::write(sp,boost::asio::buffer("\x02",1));
    boost::asio::write(sp,boost::asio::buffer("\xFc",1));
    fout.close();
    foutlaser.close();
    sp.close();
    return 0;
}



