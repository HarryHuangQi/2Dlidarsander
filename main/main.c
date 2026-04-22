#include <stdio.h>

#include "WIFI.h"
#include "UART.h"
#include "UDP_TCP.h"
#include "LD14.h"
#include "Data_declaration.h"

/**
 * @brief 应用程序主入口点
 * 
 * 启动顺序:
 * 1. WiFi初始化 (STA模式, 静态IP 192.168.43.105)
 * 2. UART初始化 (UART1, 230400波特率, 接收LD14数据)
 * 3. UDP/TCP初始化 (创建UDP socket和TCP监控任务)
 * 4. 设置init_ok标志 (为传感器任务开启数据处理)
 * 5. LD14初始化 (创建LD14_data_Task, 开始读取传感器数据)
 * 
 * 数据流:
 * LD14 (UART1) → UART驱动 → LD14_data_Task 
 *              → CRC验证 → udp_write_LD14() 
 *              → 检查TCP连接状态 → UDP转发到PC(192.168.43.250:8888)
 */
void app_main(void)
{
	/* 第1步: 初始化WiFi (STA模式, 阻塞直到连接成功或失败) */
	printf("=== Initializing WiFi (STA mode) ===\n");
	if (wifi_init_sta()) {
		printf("✓ WiFi connected!\n\n");
	} else {
		printf("✗ WiFi connect failed!\n\n");
	}

	/* 第2步: 初始化UART1用于LD14激光雷达 */
	printf("=== Initializing UART1 (LD14 sensor) ===\n");
	UART_init();
	printf("✓ UART1 ready (230400 baud, GPIO17/GPIO18)\n\n");

	/* 第3步: 初始化UDP/TCP通信 */
	printf("=== Initializing UDP/TCP communication ===\n");
	UDP_init();
	printf("✓ UDP socket created\n");
	printf("✓ TCP monitor task started (heartbeat to 192.168.43.250:8888)\n\n");

	/* 第4步: 设置初始化完成标志 (允许传感器任务开始处理数据) */
	init_ok = true;
	printf("=== init_ok = true (gate opened for sensor tasks) ===\n\n");

	/* 第5步: 初始化LD14传感器 */
	printf("=== Initializing LD14 sensor ===\n");
	LD14_init();
	printf("✓ LD14_data_Task created and running\n\n");

	printf("=== System startup complete ===\n");
	printf("Data flow: LD14 → UART1 → CRC validate → UDP forward → PC\n");
	printf("TCP connection: monitoring heartbeat\n");
}
