/*
 * HMC5883L.h
 *
 *  Created on: 2023年1月8日
 *      Author: liguanxi
 */

#ifndef LD14_H_
#define LD14_H_

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "Data_declaration.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "freertos/queue.h"

//extern QueueHandle_t LD14_queueMsg;


void LD14_init(void);


#endif /* COMPONENTS_SENSOR_HMC5883L_HMC5883L_H_ */
