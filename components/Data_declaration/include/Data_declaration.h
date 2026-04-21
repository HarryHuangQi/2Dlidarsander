/*
 * Data declaration.h
 *
 *  Created on: 2023年1月8日
 *      Author: liguanxi
 */

#ifndef DATA_DECLARATION_H_
#define DATA_DECLARATION_H_

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "esp_system.h"
#include "esp_log.h"
#include "string.h"
#include "math.h"

extern bool init_ok;

extern uint32_t VBAT;
extern int64_t task_run_time_us[10];

#endif /* COMPONENTS_DATA_DECLARATION_DATA_DECLARATION_H_ */
