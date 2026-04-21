/*
 * UART1.h
 *
 *  Created on: 2022年11月14日
 *      Author: admin
 */

#ifndef _UART_H_
#define _UART_H_

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

static const int RX_BUF_SIZE = 1024;//接收缓冲区大小

//#define TXD_PIN    3//(GPIO_NUM_4)//发送引脚 43  19
//#define RXD_PIN    9//(GPIO_NUM_5)//接收引脚 44   8

void UART_init(void);//复位函数输入波特率
void tx_task(void *arg);//发送函数
void rx_task(void *arg);//接送函数
int sendData( const char* data);//发送函数
void UART_write_anotc(uint8_t *data,uint8_t len);

#endif /* COMPONENTS_UART_INCLUDE_UART1_H_ */
