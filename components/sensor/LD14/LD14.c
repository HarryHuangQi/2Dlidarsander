/*
 * HMC5883L.c
 *  磁力计
 *  Created on: 2023年1月8日
 *      Author: liguanxi
 */
#include "include/LD14.h"

#include "UART.h"//串口头文件

#include <stdio.h>
#include "esp_log.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "esp_system.h"

#include "Data_declaration.h" //数据声明头文件

#include "UDP_TCP.h" //UDP通信头文件

typedef struct { //LD14激光雷达解码数据结构体
  uint16_t rotate_speed; //雷达转速
  uint16_t Initial_Angle; //雷达起始角度
  uint16_t end_Angle; //雷达结束角度
  uint16_t timestamp; //时间戳
} LD14_radar_t;

LD14_radar_t LD14_radar_data; //LD14数据结构体

uint8_t LD14_data_buf[47] = { 0 };  //LD14数据缓冲区
uint8_t YDLIDAR_X2_data_buf[200] = { 0 };  //LD14数据缓冲区

static const uint8_t CrcTable[256] = {//CRC校验表
  0x00, 0x4d, 0x9a, 0xd7, 0x79, 0x34, 0xe3, 0xae, 0xf2, 0xbf, 0x68, 0x25,
  0x8b, 0xc6, 0x11, 0x5c, 0xa9, 0xe4, 0x33, 0x7e, 0xd0, 0x9d, 0x4a, 0x07,
  0x5b, 0x16, 0xc1, 0x8c, 0x22, 0x6f, 0xb8, 0xf5, 0x1f, 0x52, 0x85, 0xc8,
  0x66, 0x2b, 0xfc, 0xb1, 0xed, 0xa0, 0x77, 0x3a, 0x94, 0xd9, 0x0e, 0x43,
  0xb6, 0xfb, 0x2c, 0x61, 0xcf, 0x82, 0x55, 0x18, 0x44, 0x09, 0xde, 0x93,
  0x3d, 0x70, 0xa7, 0xea, 0x3e, 0x73, 0xa4, 0xe9, 0x47, 0x0a, 0xdd, 0x90,
  0xcc, 0x81, 0x56, 0x1b, 0xb5, 0xf8, 0x2f, 0x62, 0x97, 0xda, 0x0d, 0x40,
  0xee, 0xa3, 0x74, 0x39, 0x65, 0x28, 0xff, 0xb2, 0x1c, 0x51, 0x86, 0xcb,
  0x21, 0x6c, 0xbb, 0xf6, 0x58, 0x15, 0xc2, 0x8f, 0xd3, 0x9e, 0x49, 0x04,
  0xaa, 0xe7, 0x30, 0x7d, 0x88, 0xc5, 0x12, 0x5f, 0xf1, 0xbc, 0x6b, 0x26,
  0x7a, 0x37, 0xe0, 0xad, 0x03, 0x4e, 0x99, 0xd4, 0x7c, 0x31, 0xe6, 0xab,
  0x05, 0x48, 0x9f, 0xd2, 0x8e, 0xc3, 0x14, 0x59, 0xf7, 0xba, 0x6d, 0x20,
  0xd5, 0x98, 0x4f, 0x02, 0xac, 0xe1, 0x36, 0x7b, 0x27, 0x6a, 0xbd, 0xf0,
  0x5e, 0x13, 0xc4, 0x89, 0x63, 0x2e, 0xf9, 0xb4, 0x1a, 0x57, 0x80, 0xcd,
  0x91, 0xdc, 0x0b, 0x46, 0xe8, 0xa5, 0x72, 0x3f, 0xca, 0x87, 0x50, 0x1d,
  0xb3, 0xfe, 0x29, 0x64, 0x38, 0x75, 0xa2, 0xef, 0x41, 0x0c, 0xdb, 0x96,
  0x42, 0x0f, 0xd8, 0x95, 0x3b, 0x76, 0xa1, 0xec, 0xb0, 0xfd, 0x2a, 0x67,
  0xc9, 0x84, 0x53, 0x1e, 0xeb, 0xa6, 0x71, 0x3c, 0x92, 0xdf, 0x08, 0x45,
  0x19, 0x54, 0x83, 0xce, 0x60, 0x2d, 0xfa, 0xb7, 0x5d, 0x10, 0xc7, 0x8a,
  0x24, 0x69, 0xbe, 0xf3, 0xaf, 0xe2, 0x35, 0x78, 0xd6, 0x9b, 0x4c, 0x01,
  0xf4, 0xb9, 0x6e, 0x23, 0x8d, 0xc0, 0x17, 0x5a, 0x06, 0x4b, 0x9c, 0xd1,
  0x7f, 0x32, 0xe5, 0xa8
};

uint8_t CalCRC8(const uint8_t* data, uint16_t data_len) { //CRC校验
  uint8_t crc = 0;
  while (data_len--) {
    crc = CrcTable[(crc ^ *data) & 0xff];
    data++;
  }
  return crc;
}


int LD14_test(void){ //LD14激光雷达检测
	uint8_t re_Buf[47] = {0};
	const int rxBytes = uart_read_bytes(UART_NUM_1, re_Buf, 47, 0);

	printf(" 接收到 %d 字节  \n",rxBytes);
	if (rxBytes >= 47) {//如果读取到数据就进行校验
		for(int i = 0 ; i < 47 ; i++){
			printf("%x ",re_Buf[i]);
		}
		printf("\n");
		if(CalCRC8(re_Buf, 46) == re_Buf[46]) {//CRC校验
			printf("CRC校验 成功\n");
			return ESP_OK;
		}else{
			printf("CRC校验 失败\n");
			uart_flush(UART_NUM_1);
			return ESP_FAIL;
		}
	}
	return ESP_FAIL;
}

int YDLIDAR_X2_test(void){ //YDLIDAR X2激光雷达检测
	uint8_t re_Buf[80] = {0};
	const int rxBytes = uart_read_bytes(UART_NUM_1, re_Buf, 60, 0);

	printf(" 接收到 %d 字节  \n",rxBytes);
	if (rxBytes >= 47) {//如果读取到数据就进行校验
		for(int i = 0 ; i < 60 ; i++){
			printf("%x ",re_Buf[i]);
		}
		printf("\n");
		if(re_Buf[0] == 0xAA && re_Buf[1] == 0x55) {//CRC校验
			printf("CRC校验 成功\n");
			return ESP_OK;
		}else{
			printf("CRC校验 失败\n");
			uart_flush(UART_NUM_1);
			return ESP_FAIL;
		}
	}
	return ESP_FAIL;
}


void LD14_data_Task(void *pvParameter)
{
	while (1) {
		if(init_ok){

			const int rxBytes = uart_read_bytes(UART_NUM_1, LD14_data_buf, 47, 0);//读取串口数据
			//printf("接收到 %d 字节 \n",rxBytes);


			if (rxBytes > 0) {//如果读取到数据就进行校验
				//printf("  接受到 %d 字节 \n",rxBytes);

	            if (CalCRC8(LD14_data_buf, 46) == LD14_data_buf[46]) {//CRC校验
//	            	LD14_radar_data.rotate_speed = LD14_data_buf[3] << 8 | LD14_data_buf[2]; //雷达转速
//	            	LD14_radar_data.Initial_Angle = LD14_data_buf[5] << 8 | LD14_data_buf[4];//雷达起始角度
//	            	LD14_radar_data.end_Angle = LD14_data_buf[43] << 8 | LD14_data_buf[42];  //雷达结束角度
//	            	LD14_radar_data.timestamp = LD14_data_buf[45] << 8 | LD14_data_buf[44];  //时间戳
	            	udp_write_LD14(LD14_data_buf,47);
	            	//printf("CRC解析 成功\n");
	                //printf("雷达数据：转速= %d  起始角度= %d  结束角度= %d  时间戳= %d  \n",LD14_radar_data.rotate_speed,LD14_radar_data.Initial_Angle,LD14_radar_data.end_Angle,LD14_radar_data.timestamp);
	            } else {
	              uart_flush(UART_NUM_1);
	              //printf("CRC解析 失败\n");
	            }
	       }
	   }
			vTaskDelay(1 / portTICK_PERIOD_MS); //读取后释放1个周期给看门狗复位
	}
}

void YDLIDAR_X2_data_Task(void *pvParameter)
{
	while (1) {
		if(init_ok){

			const int rxBytes = uart_read_bytes(UART_NUM_1, YDLIDAR_X2_data_buf,200,pdMS_TO_TICKS(0));//读取串口数据

//			printf(" 接收到 %d 字节  \n",rxBytes);
//			if (rxBytes >= 47) {//如果读取到数据就进行校验
//				for(int i = 0 ; i < 60 ; i++){
//					printf("%x ",YDLIDAR_X2_data_buf[i]);
//				}
//				printf("\n");
//		}


			if (rxBytes > 0) {//如果读取到数据就进行校验
				//printf("  接受到 %d 字节 \n",rxBytes);
				//printf("%x ",YDLIDAR_X2_data_buf[0]);
				//printf("%x \n",YDLIDAR_X2_data_buf[1]);

				//if(YDLIDAR_X2_data_buf[0] == 0xAA && YDLIDAR_X2_data_buf[1] == 0x55 && rxBytes == 60) {//CRC校验
//	            	LD14_radar_data.rotate_speed = LD14_data_buf[3] << 8 | LD14_data_buf[2]; //雷达转速
//	            	LD14_radar_data.Initial_Angle = LD14_data_buf[5] << 8 | LD14_data_buf[4];//雷达起始角度
//	            	LD14_radar_data.end_Angle = LD14_data_buf[43] << 8 | LD14_data_buf[42];  //雷达结束角度
//	            	LD14_radar_data.timestamp = LD14_data_buf[45] << 8 | LD14_data_buf[44];  //时间戳
	            	udp_write_LD14(YDLIDAR_X2_data_buf,rxBytes);
	            //	/printf("解析 成功\n");
	                //printf("雷达数据：转速= %d  起始角度= %d  结束角度= %d  时间戳= %d  \n",LD14_radar_data.rotate_speed,LD14_radar_data.Initial_Angle,LD14_radar_data.end_Angle,LD14_radar_data.timestamp);
	            //} else {
	            //  uart_flush(UART_NUM_1);
	            //  printf("解析 失败\n");
	            //}
	       }
	   }
			vTaskDelay(1 / portTICK_PERIOD_MS); //读取后释放1个周期给看门狗复位
	}
}




void LD14_init(void){

	printf("\n********************LD14初始化开始********************\n");

//	int err = 0;
//	//LD14激光雷达检测
//	printf("正在获取LD14数据并校验\n");
//	while(err <= 5){
//		if(YDLIDAR_X2_test() == ESP_OK) break;
//		else err++;
//		printf("第%d次重试\n",err);
//	}

//	if(err >= 5){
//		printf("LD14初始化 失败\n");
//	}else{
		//xTaskCreate(LD14_data_Task, "LD14_data_Task", 4096 * 8, NULL, 15, NULL);
		xTaskCreate(YDLIDAR_X2_data_Task, "YDLIDAR_X2_data_Task", 4096 * 8, NULL, 15, NULL);
//		printf("创建LD14雷达数据转发任务 LD14_data_Task \n");
//		printf("LD14初始化 成功\n");
//	}

	printf("********************LD14初始化完成********************\n\n");


}

