/*
 * UDP.h
 *
 *  Created on: 2023年2月7日
 *      Author: liguanxi
 */

#ifndef COMPONENTS_EXTERNAL_COMMUNICATION_UDP_TCP_INCLUDE_UDP_TCP_H_
#define COMPONENTS_EXTERNAL_COMMUNICATION_UDP_TCP_INCLUDE_UDP_TCP_H_

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

void UDP_init(void);

void udp_write_LD14(uint8_t *data,uint8_t len);//LD14激光雷达DUP发送

#endif /* COMPONENTS_EXTERNAL_COMMUNICATION_UDP_TCP_INCLUDE_UDP_TCP_H_ */
