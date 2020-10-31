#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include "ros/ros.h"
#include "std_msgs/MultiArrayLayout.h"
#include "std_msgs/MultiArrayDimension.h"
#include "std_msgs/Float32MultiArray.h"
#include "std_msgs/Float32.h"
#define LISTEN 0
#define ACK 1
const int MAXCONNECTIONS =5;
const int MAXSENDBUFSIZE = 1024;
const int MAXRECEIVEBUFSIZE = 1024;
double l_speed_data =0;
double r_speed_data =0;

bool is_msg_grab = false;
using namespace std;
struct Recieve
{
	int mode;
	int todo;
	double l_speed,r_speed;
	double PID[2][3]{};
}Reciev_data;

struct Send
{
	double data1;
	double data2;
	double x;
	double y;
	double w;

}Send_data;

void CallBack_1(const std_msgs::Float32 &msg);
void CallBack_2(const std_msgs::Float32 &msg);
void CallBack_3(const std_msgs::Float32MultiArray::ConstPtr& msg);
int main(int argc,char**argv){
	ros::init(argc,argv,"SOCKET_NODE");
	ros::NodeHandle nh;
	ros::Subscriber sub1 = nh.subscribe("/L_speed",100,CallBack_1);
	ros::Subscriber sub2 = nh.subscribe("/R_speed",100,CallBack_2);
	ros::Subscriber sub3 = nh.subscribe("/robot_pose",100,CallBack_3);
	ros::Publisher pub = nh.advertise<std_msgs::Float32MultiArray>("/speed_set", 1000);
	std_msgs::Float32MultiArray pub_msg;
	pub_msg.data.clear();
	int listenSockFD;
	int clientSockFD;
	sockaddr_in server_addr,client_addr;
	listenSockFD = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(listenSockFD<0){
		cout<<endl<<"socket create error"<<endl;
		return 0;
	}
	int on =1;
	if(setsockopt(listenSockFD,SOL_SOCKET,SO_REUSEADDR,(const char*)&on,sizeof(on))<0){
		cout<<endl<<"set option curLen =0 , Error"<<endl;
		return 0;
	}
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(30000);
	if(bind(listenSockFD,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
		cout<<endl<<"bind error"<<endl;
		return 0;
	}
	cout<<"server running. wait client..."<<endl;
	if(listen(listenSockFD,MAXCONNECTIONS)<0){
		cout<<endl<<"listen error"<<endl;
		return 0;
	}
	int clientAddrSize = sizeof(client_addr);
	while(true){
		clientSockFD = accept(listenSockFD,(struct sockaddr*) &client_addr,(socklen_t*) &clientAddrSize);
		if(clientSockFD<0){
			cout<<endl<<"accept error"<<endl;
			return 0;
		}
		cout<<endl<<"client accepted"<<endl;
		cout<<"address : "<<inet_ntoa(client_addr.sin_addr)<<endl;
		cout<<"port : "<<ntohs(client_addr.sin_port)<<endl;
		while(ros::ok()){
			if(recv(clientSockFD,(char*)&Reciev_data,sizeof(Reciev_data),0)<0){
				cout<<endl<<"receive errer"<<endl;
			}
			if(Reciev_data.mode == LISTEN){
				if(is_msg_grab){
					Send_data.data1 = l_speed_data;
					Send_data.data2 = r_speed_data;
				}
				if(send(clientSockFD,(char*)&Send_data,sizeof(Send_data),0)<0)
					cout<<endl<<"send error"<<endl;
				is_msg_grab = false;
			}
			else if(Reciev_data.mode == ACK){
				if(Reciev_data.todo == 0){
					pub_msg.data.clear();
					pub_msg.data.push_back(0);
					pub_msg.data.push_back(0);
					pub_msg.data.push_back(0);
					pub_msg.data.push_back(0);
					pub_msg.data.push_back(0);
					pub_msg.data.push_back(0);
					pub_msg.data.push_back(0);
					pub.publish(pub_msg);
				}
				else if(Reciev_data.todo == 1){
					pub_msg.data.clear();
					pub_msg.data.push_back(Reciev_data.l_speed);
					pub_msg.data.push_back(Reciev_data.r_speed);
					pub_msg.data.push_back(Reciev_data.PID[0][0]);
					pub_msg.data.push_back(Reciev_data.PID[0][1]);
					pub_msg.data.push_back(Reciev_data.PID[0][2]);
					pub_msg.data.push_back(Reciev_data.PID[1][0]);
					pub_msg.data.push_back(Reciev_data.PID[1][1]);
					pub_msg.data.push_back(Reciev_data.PID[1][2]);
					pub.publish(pub_msg);
				}
				else
					break;
				cout<<"[ TODO ] = "<<Reciev_data.todo<<endl;
				cout<<"motor speed = "<<Reciev_data.l_speed<<" , "<<Reciev_data.r_speed<<endl;
				cout<<"[ PID1 ] = "<<Reciev_data.PID[0][0]<<" , "<<Reciev_data.PID[0][1]<<" , "<<Reciev_data.PID[0][2]<<endl;
				cout<<"[ PID2 ] = "<<Reciev_data.PID[1][0]<<" , "<<Reciev_data.PID[1][1]<<" , "<<Reciev_data.PID[1][2]<<endl;
			}
			ros::spinOnce();
		}
		cout<<endl<<"client closed "<<endl;
		cout<<"address : "<<inet_ntoa(client_addr.sin_addr)<<endl;
		cout<<"port : "<<ntohs(client_addr.sin_port)<<endl;
	}
	close(listenSockFD);
	cout<<"server end"<<endl;
	return 0;
}

void CallBack_1(const std_msgs::Float32 &msg){
	l_speed_data = msg.data;
	is_msg_grab = true;
}

void CallBack_2(const std_msgs::Float32 &msg){
	r_speed_data = msg.data;
	is_msg_grab = true;
}

void CallBack_3(const std_msgs::Float32MultiArray::ConstPtr& msg){
	Send_data.x = msg->data[0];
	Send_data.y = msg->data[1];
	Send_data.w = msg->data[2];
}