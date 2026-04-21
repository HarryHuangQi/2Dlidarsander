/*
 * Data declaration.c
 *  数据声明，结构体，堆栈空间，飞行器姿态和PID各类数据在此声明
 *  Created on: 2023年1月8日
 *      Author: liguanxi
 */

#include "Data_declaration.h"

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "esp_system.h"
#include "esp_log.h"
#include "string.h"
#include "math.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"

bool init_ok = false;//初始化成功后置1全部任务才会开始运行
int64_t task_run_time_us[10] = {0};  //系统运行时间
uint32_t VBAT;  //电压

