/*
 * WIFI.c
 *
 *  Created on: 2023年1月25日
 *      Author: liguanxi
 */

#include "WIFI.h"

//#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"





#define CONFIG_ESP_MAXIMUM_RETRY 10

#define EXAMPLE_ESP_WIFI_SSID      "BIRD 9407" //wifi名称
#define EXAMPLE_ESP_WIFI_PASS      "wssn2233"//WiFi密码

//#define EXAMPLE_ESP_WIFI_SSID      "redmi" //wifi名称
//#define EXAMPLE_ESP_WIFI_PASS      "1234567890"//WiFi密码

//#define EXAMPLE_ESP_WIFI_SSID      "xiaomi" //wifi名称
//#define EXAMPLE_ESP_WIFI_PASS      "12345678"//WiFi密码

#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY//最大重试次数5

//#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
//#endif

/* FreeRTOS事件组当我们连接时发出信号*/
static EventGroupHandle_t s_wifi_event_group;

/*事件组允许每个事件有多个位，但我们只关心两个事件:
 * -我们通过IP连接到AP
 * -我们在最大重试次数后未能连接*/

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;

#define PORT 3333 //端口号


/* Static IP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include <netdb.h>
#include "nvs_flash.h"

/* The examples use configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_WIFI_SSID             CONFIG_EXAMPLE_WIFI_SSID
#define EXAMPLE_WIFI_PASS             CONFIG_EXAMPLE_WIFI_PASSWORD
#define EXAMPLE_MAXIMUM_RETRY         "5"
#define EXAMPLE_STATIC_IP_ADDR        "192.168.43.105"
#define EXAMPLE_STATIC_NETMASK_ADDR   "255.255.255.0"
#define EXAMPLE_STATIC_GW_ADDR        "192.168.43.1"

#define EXAMPLE_MAIN_DNS_SERVER       "192.168.43.1"
#define EXAMPLE_BACKUP_DNS_SERVER     "0.0.0.0"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1



static esp_err_t example_set_dns_server(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type)
{
    if (addr && (addr != IPADDR_NONE)) {
        esp_netif_dns_info_t dns;
        dns.ip.u_addr.ip4.addr = addr;
        dns.ip.type = IPADDR_TYPE_V4;
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, type, &dns));
    }
    return ESP_OK;
}

static void example_set_static_ip(esp_netif_t *netif)
{
    if (esp_netif_dhcpc_stop(netif) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop dhcp client");
        return;
    }
    esp_netif_ip_info_t ip;
    memset(&ip, 0 , sizeof(esp_netif_ip_info_t));
    ip.ip.addr = ipaddr_addr(EXAMPLE_STATIC_IP_ADDR);
    ip.netmask.addr = ipaddr_addr(EXAMPLE_STATIC_NETMASK_ADDR);
    ip.gw.addr = ipaddr_addr(EXAMPLE_STATIC_GW_ADDR);
    if (esp_netif_set_ip_info(netif, &ip) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set ip info");
        return;
    }
    ESP_LOGD(TAG, "Success to set static ip: %s, netmask: %s, gw: %s", EXAMPLE_STATIC_IP_ADDR, EXAMPLE_STATIC_NETMASK_ADDR, EXAMPLE_STATIC_GW_ADDR);
    ESP_ERROR_CHECK(example_set_dns_server(netif, ipaddr_addr(EXAMPLE_MAIN_DNS_SERVER), ESP_NETIF_DNS_MAIN));
    ESP_ERROR_CHECK(example_set_dns_server(netif, ipaddr_addr(EXAMPLE_BACKUP_DNS_SERVER), ESP_NETIF_DNS_BACKUP));
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        example_set_static_ip(arg);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        s_retry_num++;
        ESP_LOGI(TAG, "retry to connect to the AP (%d)", s_retry_num);
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "static ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

//static void event_handler(void* arg, esp_event_base_t event_base,
//                                int32_t event_id, void* event_data)
//{
//    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
//        esp_wifi_connect();
//    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
//        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
//            esp_wifi_connect();
//            s_retry_num++;
//            ESP_LOGI(TAG, "retry to connect to the AP");
//        } else {
//            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
//        }
//        ESP_LOGI(TAG,"connect to the AP fail");
//    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
//        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
//        s_retry_num = 0;
//        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
//    }
//}

bool wifi_init_sta(void)
{
	printf("\n\n********************WiFi初始化开始********************\n");

	printf("默认NVS分区初始化 ");//初始化默认 NVS 分区
	if(nvs_flash_init() == ESP_OK) printf("成功\n");
	else printf("失败\n");

	printf("创建新的事件组 ");
    s_wifi_event_group = xEventGroupCreate();//创建新的事件组,返回事件组的句柄
    if(s_wifi_event_group != ESP_OK) printf("成功\n");//初始化默认 NVS 分区
    else printf("失败\n");

    printf("初始化底层 TCP/IP 堆栈 ");//初始化底层 TCP/IP 堆栈
    if(esp_netif_init() == ESP_OK) printf("成功\n");
    else printf("失败\n");

    printf("创建默认事件循环 ");//创建默认事件循环
    if(esp_event_loop_create_default() == ESP_OK) printf("成功\n");//创建默认事件循环
    else printf("失败\n");

    printf("创建默认无线网络。如果出现任何初始化错误，此 API 将中止\n");//创建默认事件循环

    esp_netif_create_default_wifi_sta();//创建默认无线网络。如果出现任何初始化错误，此 API 将中止


    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); //简单的WiFi堆栈配置参数传递给esp_wifi_init调用。

    //初始化WiFi为WiFi驱动程序分配资源，如WiFi控制结构，RX/TX缓冲区，WiFi NVS结构等。此 WiFi 也会启动 WiFi 任务
    printf("初始化WiFi并为WiFi驱动程序分配资源\n");
    printf("//////////////////////////////////////////////////\n");
    if(esp_wifi_init(&cfg) == ESP_OK) printf("成功\n");
    else printf("失败\n");
    printf("//////////////////////////////////////////////////\n\n");
    //*************************************************************//
    //这段代码大概的意思是声明一个函数event_handler有连接或断开操作时执行这个函数  将事件处理程序的实例注册到默认循环。

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    //将事件处理程序的实例注册到默认循环
    printf("将instance_any_id事件处理程序的实例注册到默认循环 ");
    if(esp_event_handler_instance_register(WIFI_EVENT,//要为其注册处理程序的事件的基本 ID
                                           ESP_EVENT_ANY_ID,//要为其注册处理程序的事件的 ID
                                           &event_handler,//调度事件时调用的处理程序函数
                                           NULL,//数据，除了事件数据，在调用处理程序时传递给处理程序
                                           &instance_any_id//任务的操作句柄
										   ) == ESP_OK) printf("成功\n");
    else printf("失败\n");
    //将事件处理程序的实例注册到默认循环
    printf("将instance_got_ip事件处理程序的实例注册到默认循环 ");
    if(esp_event_handler_instance_register(IP_EVENT,
                                           IP_EVENT_STA_GOT_IP,
                                           &event_handler,
                                           NULL,
                                           &instance_got_ip
										   ) == ESP_OK) printf("成功\n");
    else printf("失败\n");

    //**********************************************************************************//

    wifi_config_t wifi_config = {//声明WiFi参数结构体
        .sta = {//使用sta模式
            .ssid = EXAMPLE_ESP_WIFI_SSID,//wifi名称
            .password = EXAMPLE_ESP_WIFI_PASS,//wifi密码
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD, //认证模式，打开
        },
    };

    printf("设置WiFi模式为STA ");
    if(esp_wifi_set_mode(WIFI_MODE_STA) == ESP_OK) printf("成功\n");//设置 WiFi 操作模式为sta
    else printf("失败\n");

    printf("设置 WiFi STA 的配置参数 ");//设置 ESP32 STA 的配置
    if(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) == ESP_OK) printf("成功\n");//设置 ESP32 STA 的配置。
    else printf("失败\n");

    printf("WiFi名称:%s\n",EXAMPLE_ESP_WIFI_SSID);
    printf("WiFi密码:%s\n",EXAMPLE_ESP_WIFI_PASS);

    //根据当前配置启动WiFi 如果模式WIFI_MODE_STA，则创建站控制块并启动站
    printf("根据当前配置启动WiFi和创建站控制块并启动站\n");
    printf("//////////////////////////////////////////////////\n");
    if(esp_wifi_start() == ESP_OK) printf("成功\n");
    else printf("失败\n");
    printf("//////////////////////////////////////////////////\n\n");

    /* 等待连接建立(WIFI_CONNECTED_BIT)或最大连接失败
     *重试次数(WIFI_FAIL_BIT)。位由event_handler()(见上文)设置*/
    printf("正在连接WiFi(将持续重试直到成功)....\n");
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
               						   WIFI_CONNECTED_BIT,
										   pdFALSE,
										   pdFALSE,
										   portMAX_DELAY);

    /* xEventGroupWaitBits()返回返回调用之前的位，因此我们可以测试实际是哪个事件
     *发生*/

    bool wifi_connected = false;
    if (bits & WIFI_CONNECTED_BIT) {
    	printf("WIFI连接 成功\n");
        wifi_connected = true;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        printf("其他错误导致 失败\n");
    }
    printf("\n********************WiFi初始化结束********************\n\n");
    return wifi_connected;
}

