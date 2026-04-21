/*
 * Data declaration.c
 *  数据声明，结构体，堆栈空间，飞行器姿态和PID各类数据在此声明
 *  Created on: 2023年1月8日
 *      Author: liguanxi
 */

#include "include/nvs_storage.h"

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "esp_system.h"
#include "esp_log.h"
#include "string.h"
#include "math.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

