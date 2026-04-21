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

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include <lwip/netdb.h>

#include <sys/param.h>
#include "esp_netif.h"

#include "LD14.h"



//LD14激光雷达IP地址

static int sock_pc = -1;//PC端UDP套接字
static struct sockaddr_in dest_addr_pc;//电脑端地址
static int sock_tcp = -1;//TCP监测套接字

const int PC_PORT = 8888; //电脑端口
const int TCP_PORT = 8888; //TCP监测端口


bool udp_state = false; //UDP连接状态
bool tcp_state = false; //TCP连接状态

int addr_family = 2; //套接字要使用的协议簇 AF_INET
int ip_protocol = 0; //确定套接字的协议簇和类型时这个参数为0

#define CONFIG_EXAMPLE_ADDR "192.168.43.250"

#define HOST_IP_ADDR CONFIG_EXAMPLE_ADDR //IP地址

static void tcp_monitor_task(void *pvParameters)
{
    char host_ip[] = HOST_IP_ADDR;
    const char *heartbeat = "ESP32_TCP_MONITOR\n";

    while (1) {
        struct sockaddr_in dest_addr_tcp;
        memset(&dest_addr_tcp, 0, sizeof(dest_addr_tcp));
        dest_addr_tcp.sin_addr.s_addr = inet_addr(host_ip);
        dest_addr_tcp.sin_family = AF_INET;
        dest_addr_tcp.sin_port = htons(TCP_PORT);

        sock_tcp = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (sock_tcp < 0) {
            tcp_state = false;
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        int err = connect(sock_tcp, (struct sockaddr *)&dest_addr_tcp, sizeof(dest_addr_tcp));
        if (err != 0) {
            tcp_state = false;
            shutdown(sock_tcp, 0);
            close(sock_tcp);
            sock_tcp = -1;
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        tcp_state = true;
        printf("TCP监测连接 成功, 目标IP:%s 端口:%d\n", HOST_IP_ADDR, TCP_PORT);

        while (1) {
            int sent = send(sock_tcp, heartbeat, strlen(heartbeat), 0);
            if (sent < 0) {
                tcp_state = false;
                break;
            }
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        if (sock_tcp != -1) {
            shutdown(sock_tcp, 0);
            close(sock_tcp);
            sock_tcp = -1;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

static void udp_socket_init(void)
{
    if (sock_pc >= 0) {
        shutdown(sock_pc, 0);
        close(sock_pc);
        sock_pc = -1;
    }

    memset(&dest_addr_pc, 0, sizeof(dest_addr_pc));
    dest_addr_pc.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
    dest_addr_pc.sin_family = AF_INET;
    dest_addr_pc.sin_port = htons(PC_PORT);

    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;

    sock_pc = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock_pc < 0) {
        printf("UDP套接字创建 失败\n");
        udp_state = false;
        return;
    }

    udp_state = true;
    printf("UDP套接字创建 成功, 目标IP:%s 端口:%d\n", HOST_IP_ADDR, PC_PORT);
}

void udp_write_LD14(uint8_t *data,uint8_t len){//LD14激光雷达DUP发送
    if (!tcp_state) {
        return;
    }

    if (!udp_state || sock_pc < 0) {
        udp_socket_init();
    }

    if (!udp_state || sock_pc < 0) {
        return;
    }

    //发送数据
    int err = sendto(sock_pc,
                     data,
                     len,
                     0,
                     (struct sockaddr *)&dest_addr_pc,
                     sizeof(dest_addr_pc));
    if (err < 0) {
        udp_state = false;//发送失败
        udp_socket_init();//失败后立即尝试重建，下一包恢复发送
    }
    else udp_state = true;//发送成功

}

void UDP_init(void){//UDP通信初始化

	printf("*******************UDP初始化开始*******************\n");

	udp_socket_init();
    xTaskCreate(tcp_monitor_task, "tcp_monitor_task", 4096, NULL, 10, NULL);

	printf("*******************UDP初始化结束*******************\n");

}

