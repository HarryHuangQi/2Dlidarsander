/*
 * UDP_TCP.c
 *
 *  Created on: 2023年2月7日
 *      Author: liguanxi
 */

#include "UDP_TCP.h"

#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "Data_declaration.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include <lwip/netdb.h>

#include <sys/param.h>
#include "esp_netif.h"

#include "LD14.h"



//LD14激光雷达IP地址

static void udp_anotc_read_task(void *pvParameters); //匿名地面站接收任务
static void udp_rc_read_task(void *pvParameters); //遥控器接收任务
static void udp_ros_read_task(void *pvParameters); //ROS系统接收任务

struct sockaddr_storage source_addr_anotc; //地面站套接字地址存储变量足以支持IPV6
struct sockaddr_in6 dest_addr_anotc; //地面站套接字地址
socklen_t socklen_anotc = sizeof(source_addr_anotc);//计算地面站存储变量大小

struct sockaddr_storage source_addr_rc; //遥控器套接字地址存储变量足以支持IPV6
struct sockaddr_in6 dest_addr_rc; //遥控器套接字地址
socklen_t socklen_rc = sizeof(source_addr_rc);//计算遥控器存储变量大小

struct sockaddr_storage source_addr_ros; //遥控器套接字地址存储变量足以支持IPV6
struct sockaddr_in6 dest_addr_ros; //遥控器套接字地址
socklen_t socklen_ros = sizeof(source_addr_ros);//计算遥控器存储变量大小

struct sockaddr_in dest_addr;//激光雷达套接字IP存储

char rx_buffer_anotc[128];//地面站接收缓冲区
char addr_str_anotc[128]; //地面站设备地址

char rx_buffer_rc[128];//遥控器接收缓冲区
char addr_str_rc[128]; //遥控器设备地址

char rx_buffer_ros[128];//ROS系统接收缓冲区
char addr_str_ros[128]; //ROS系统设备地址

uint8_t vel_read_buf[50];  //速度控制数据接收缓冲区
uint8_t imu_send_buf[255];  //姿态数据发送缓冲区

int sock_anotc; //地面站套接字
int sock_rc; //遥控器套接字
int sock_LD14;//激光雷达套接字
int sock_ros;//ROS系统套接字

const int anotc_PORT = 3333; //地面站端口
const int rc_PORT = 60000; // 遥控器端口
const int ROS_PORT = 8888; //机器人系统端口
const int LD14_PORT = 5555;//LD14端口


bool anotc_state = false; //地面站连接状态
bool rc_state = false; //遥控器连接状态
bool ros_state = false; //遥控器连接状态

int addr_family = 2; //套接字要使用的协议簇 AF_INET
int ip_protocol = 0; //确定套接字的协议簇和类型时这个参数为0

#define CONFIG_EXAMPLE_ADDR "192.168.43.250"

#define HOST_IP_ADDR CONFIG_EXAMPLE_ADDR //IP地址

#define PORT 8888 //端口

static const char *TAG = "example";
static const char *payload = "Message from ESP32 ";

typedef struct _RECEIVE_DATA_ //接收ros系统的数据结构体
{
	unsigned char buffer[11];
	struct _Control_Str_
	{
		unsigned char Frame_Header; //1字节 帧头
		float X_speed;	            //4字节 x轴速度
		float Y_speed;              //4字节 y轴速度
		float Z_speed;              //4字节 z轴速度
		unsigned char Frame_Tail;   //1字节 帧尾
	}Control_Str;
}RECEIVE_DATA;



RECEIVE_DATA Receive_Data;

static void tcp_client_task(void *pvParameters)   //数据接收任务
{
    char rx_buffer[128]; //接受缓冲区
    char host_ip[] = HOST_IP_ADDR;  //ip地址
    int addr_family = 0;
    int ip_protocol = 0;

    while (1) {
    	//定义IPV4  TCP通信参数
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(host_ip);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

        //创建套接字
        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "无法创建套接字: 错误码 %d", errno);
            break;
        }
        ESP_LOGI(TAG, "成功创建套接字 IP地址%s  端口号%d", host_ip, PORT);


        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
        if (err != 0) {
            ESP_LOGE(TAG, "套接字无法连接: 错误码 %d", errno);
            break;
        }
        ESP_LOGI(TAG, "成功连接");

        while (1) {
        	//发送数据
            int err = send(sock, payload, strlen(payload), 0);
            if (err < 0) {
                ESP_LOGE(TAG, "发送时发生错误: 错误码 %d", errno);
                break;
            }

            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // 接收时发生错误
            if (len < 0) {
                ESP_LOGE(TAG, "接收失败: 错误码 %d", errno);
                break;
            }
            else {// 处理收到的数据
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG, "接受到 %d 字节的数据 来自 %s 地址:", len, host_ip);
                ESP_LOGI(TAG, "%s", rx_buffer); //内容
            }

            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "正在关闭套接字并重新启动...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

static void udp_anotc_read_task(void *pvParameters)//地面站DUP数据接送任务
{
    while (1) {
        while (1) {
            int len = recvfrom(sock_anotc, rx_buffer_anotc, sizeof(rx_buffer_anotc) - 1, 0, (struct sockaddr *)&source_addr_anotc, &socklen_anotc);//接送数据
            if (len < 0) {//数小于0就是接收错误
            	printf("接收失败\n");
            	break;//跳出这个while循环关闭套接字并重新启动
            }
            else {//如果成功接收就获取发件人的ip地址为字符串
                inet_ntoa_r(((struct sockaddr_in *)&source_addr_anotc)->sin_addr, addr_str_anotc, sizeof(addr_str_anotc) - 1);
                //printf("成功接收地面站数据\n");
            }
        }
        if (sock_anotc != -1) {
        	printf("关闭套接字并重新启动\n");
        	shutdown(sock_anotc, 0);
        	close(sock_anotc);
        }
    }
    vTaskDelete(NULL);
}

static void udp_rc_read_task(void *pvParameters)//地面站DUP数据接送任务
{
    while (1) {
        while (1) {
            int len = recvfrom(sock_rc, rx_buffer_rc, sizeof(rx_buffer_rc) - 1, 0, (struct sockaddr *)&source_addr_rc, &socklen_rc);//接送数据
            if (len < 0) {//数小于0就是接收错误
            	printf("接收失败\n");
            	break;//跳出这个while循环关闭套接字并重新启动
            }
            else {//如果成功接收就获取发件人的ip地址为字符串
                inet_ntoa_r(((struct sockaddr_in *)&source_addr_rc)->sin_addr, addr_str_rc, sizeof(addr_str_rc) - 1);
                //printf("收到数据\n");
            }
        }
        if (sock_rc != -1) {
        	printf("关闭套接字并重新启动\n");
        	shutdown(sock_rc, 0);
        	close(sock_rc);
        }
    }
    vTaskDelete(NULL);
}

static void udp_ros_read_task(void *pvParameters)//ros系统DUP数据接送任务
{
    while (1) {
        while (1) {
            int len = recvfrom(sock_ros, rx_buffer_ros, sizeof(rx_buffer_ros) - 1, 0, (struct sockaddr *)&source_addr_ros, &socklen_ros);//接送数据
            if (len < 0) {//数小于0就是接收错误
            	printf("接收失败\n");
            	break;//跳出这个while循环关闭套接字并重新启动
            }
            else {//如果成功接收就获取发件人的ip地址为字符串
                inet_ntoa_r(((struct sockaddr_in *)&source_addr_ros)->sin_addr, addr_str_ros, sizeof(addr_str_ros) - 1);
                //rc_data_decode (rx_buffer_rc);
                //printf("收到数据\n");
            }
        }
        if (sock_ros != -1) {
        	printf("关闭套接字并重新启动\n");
        	shutdown(sock_ros, 0);
        	close(sock_ros);
        }
    }
    vTaskDelete(NULL);
}

void socket_anotc_init(void){//地面站套接字初始化

	if (addr_family == AF_INET) {//IPV4
	            struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr_anotc;
	            dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);//ip地址
	            dest_addr_ip4->sin_family = AF_INET;//套接字要使用的协议簇 AF_INET = （TCP/IP – IPv4）
	            dest_addr_ip4->sin_port = htons(anotc_PORT);//端口号
	            ip_protocol = IPPROTO_IP;
	        }

	        //创建套接字，创建成功后返回套接字，创建失败返回-1，错误代码则写入“errno”中
	        sock_anotc = socket(addr_family,  //套接字要使用的协议簇 AF_INET = （TCP/IP – IPv4）
	        				SOCK_DGRAM, //套接字类型
							ip_protocol //确定套接字的协议簇和类型时这个参数为0
							);//创建套接字
	        if (sock_anotc < 0) printf("地面站UDP套接字创建 失败\n");
	        else printf("地面站UDP套接字创建 成功\n");

	        int err = bind(sock_anotc, (struct sockaddr *)&dest_addr_anotc, sizeof(dest_addr_anotc));//套接字绑定

	        if (err < 0)printf("地面站UDP套接字绑定 失败\n");
	        else printf("地面站UDP套接字绑定 成功\n");


}

void socket_rc_init(void){//遥控器套接字初始化

	if (addr_family == AF_INET) {//IPV4
	            struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr_rc;
	            dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);//ip地址
	            dest_addr_ip4->sin_family = AF_INET;//套接字要使用的协议簇 AF_INET = （TCP/IP – IPv4）
	            dest_addr_ip4->sin_port = htons(rc_PORT);//端口号
	            ip_protocol = IPPROTO_IP;
	        }

	        //创建套接字，创建成功后返回套接字，创建失败返回-1，错误代码则写入“errno”中
	        sock_rc = socket(addr_family,  //套接字要使用的协议簇 AF_INET = （TCP/IP – IPv4）
	        				SOCK_DGRAM, //套接字类型
							ip_protocol //确定套接字的协议簇和类型时这个参数为0
							);//创建套接字
	        if (sock_rc < 0) printf("遥控器UDP套接字创建 失败\n");
	        else printf("遥控器UDP套接字创建 成功\n");

	        int err = bind(sock_rc, (struct sockaddr *)&dest_addr_rc, sizeof(dest_addr_rc));//套接字绑定

	        if (err < 0)printf("遥控器UDP套接字绑定 失败\n");
	        else printf("遥控器UDP套接字绑定 成功\n");

	        printf("遥控器UDP套接字端口号：%d\n",rc_PORT);
}

void socket_ros_init(void){//ros系统套接字初始化

	if (addr_family == AF_INET) {//IPV4
	            struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr_rc;
	            dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);//ip地址
	            dest_addr_ip4->sin_family = AF_INET;//套接字要使用的协议簇 AF_INET = （TCP/IP – IPv4）
	            dest_addr_ip4->sin_port = htons(rc_PORT);//端口号
	            ip_protocol = IPPROTO_IP;
	        }

	        //创建套接字，创建成功后返回套接字，创建失败返回-1，错误代码则写入“errno”中
	        sock_ros = socket(addr_family,  //套接字要使用的协议簇 AF_INET = （TCP/IP – IPv4）
	        				SOCK_DGRAM, //套接字类型
							ip_protocol //确定套接字的协议簇和类型时这个参数为0
							);//创建套接字
	        if (sock_rc < 0) printf("ros系统UDP套接字创建 失败\n");
	        else printf("ros系统UDP套接字创建 成功\n");

	        int err = bind(sock_ros, (struct sockaddr *)&dest_addr_ros, sizeof(dest_addr_ros));//套接字绑定

	        if (err < 0)printf("ros系统UDP套接字绑定 失败\n");
	        else printf("ros系统DP套接字绑定 成功\n");

	        printf("ros系统UDP套接字端口号：%d\n",ROS_PORT);
}

static void tcp_send_task(void *pvParameters)   //数据接收任务
{
    char rx_buffer[128]; //接受缓冲区
    char host_ip[] = HOST_IP_ADDR;  //ip地址
    int addr_family = 0;
    int ip_protocol = 0;


    while (1) {

    	//定义IPV4  TCP通信参数
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(host_ip);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(ROS_PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

        //创建套接字
        sock_ros =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock_ros < 0) printf("ROS系统TCP套接字创建 失败\n");
        else printf("ROS系统TCP套接字创建 成功\n");

        printf("成功创建套接字 IP地址%s  端口号%d \n", host_ip, ROS_PORT);


        int err = connect(sock_ros, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
        if (err != 0){
            	printf("套接字连接 失败\n");
        }else {
            	printf("套接字连接 成功\n");
        }
        while (1) {

            int len = recv(sock_ros, Receive_Data.buffer, sizeof(Receive_Data.buffer) - 1, pdMS_TO_TICKS(1));

            printf("%d  \n",len);
            // 接收时发生错误
            if (len < 0) {
            	break;
            }
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }
        if (sock_ros != -1) {
        	printf("正在关闭套接字并重新启动...\n");
            shutdown(sock_ros, 0);
            close(sock_ros);
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }
    }
    vTaskDelete(NULL);
}

void socket_LD14_init(void){


    dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR); //ip地址
    dest_addr.sin_family = AF_INET;//套接字要使用的协议簇 AF_INET = （TCP/IP – IPv4）
    dest_addr.sin_port = htons(LD14_PORT); //端口号
    addr_family = AF_INET;//套接字要使用的协议簇 AF_INET = （TCP/IP – IPv4）
    ip_protocol = IPPROTO_IP;  //确定套接字的协议簇和类型时这个参数为0

    //创建套接字，创建成功后返回套接字，创建失败返回-1，错误代码则写入“errno”中
    sock_LD14 =  socket(addr_family,  //套接字要使用的协议簇 AF_INET = （TCP/IP – IPv4）
    					SOCK_DGRAM, //套接字类型
						ip_protocol //确定套接字的协议簇和类型时这个参数为0
						);//创建套接字
    if (sock_LD14 < 0) printf("LD14激光雷达套接字创建 失败\n");
    else printf("LD14激光雷达套接字创建 成功\n");

    printf("激光雷达UDP套接字端口号：%d\n",LD14_PORT);
}


void udp_write_LD14(uint8_t *data,uint8_t len){//LD14激光雷达DUP发送
//    sendto(sock_LD14, data, len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

	//发送数据
	int err = send(sock_ros, data, len, 0);
	if (err < 0) ros_state = false;//未连接ros系统
	else ros_state = true;//已连接ros系统

}


void UDP_write_anotc(uint8_t *data,uint8_t len){//地面站DUP发送
		int err = sendto(sock_anotc, data, len, 0, (struct sockaddr *)&source_addr_anotc, sizeof(source_addr_anotc));
		if (err < 0) anotc_state = false;//未连接地面站
		else anotc_state = true;//已连接地面站
}

void UDP_write_rc(uint8_t *data,uint8_t len){//遥控器DUP发送
		int err = sendto(sock_rc, data, len, 0, (struct sockaddr *)&source_addr_rc, sizeof(source_addr_rc));
		if (err < 0) rc_state = false;//未连接遥控器
		else rc_state = true;//已连接遥控器
}

void UDP_write_ros(uint8_t *data,uint8_t len){//ros系统DUP发送
		int err = sendto(sock_ros, data, len, 0, (struct sockaddr *)&source_addr_ros, sizeof(source_addr_ros));
		if (err < 0) ros_state = false;//未连接ros系统
		else ros_state = true;//已连接ros系统
}

void UDP_init(void){//UDP通信初始化

	printf("*******************UDP初始化开始*******************\n");

	//socket_anotc_init();//创建与地面站通信的套接字
	//socket_rc_init();   //创建与遥控器通信的套接字
	//socket_LD14_init(); //创建LD14激光雷达的套接字
	//xTaskCreate(udp_anotc_read_task, "udp_anotc_read_task", 1024*8, (void*)AF_INET, 5, NULL);//创建接收地面站数据的任务
	//xTaskCreate(udp_rc_read_task, "udp_rc_read_task", 1024*8, (void*)AF_INET, 5, NULL); //创建接收遥控器数据的任务
	//xTaskCreate(udp_ros_read_task, "udp_ros_read_task", 1024*8, (void*)AF_INET, 8, NULL); //创建接收ros系统数据的任务

    //xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);  //创建tcp接受任务

	xTaskCreate(tcp_send_task, "tcp_send_task", 1024*8,(void*)AF_INET,15, NULL);  //创建发送ros系统数据的任务

	printf("*******************UDP初始化结束*******************\n");

}

