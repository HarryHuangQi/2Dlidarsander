#include <stdio.h>
#include <string.h>

#include "driver/i2c.h"
#include "esp_timer.h"

#include "WIFI.h"
#include "UART.h"
#include "UDP_TCP.h"
#include "LD14.h"
#include "Data_declaration.h"
#include "mpu_wrapper.h"

#ifndef CONFIG_IMU_I2C_SDA_GPIO
#define CONFIG_IMU_I2C_SDA_GPIO 4
#endif

#ifndef CONFIG_IMU_I2C_SCL_GPIO
#define CONFIG_IMU_I2C_SCL_GPIO 5
#endif

#define IMU_I2C_PORT I2C_NUM_0
#define IMU_I2C_FREQ_HZ 100000
#define MPU6050_WHO_AM_I_REG 0x75

static bool imu_i2c_probe_addr(uint8_t addr)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	if (cmd == NULL) {
		return false;
	}

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
	i2c_master_stop(cmd);

	esp_err_t err = i2c_master_cmd_begin(IMU_I2C_PORT, cmd, pdMS_TO_TICKS(50));
	i2c_cmd_link_delete(cmd);
	return err == ESP_OK;
}

static bool imu_i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *value)
{
	if (value == NULL) {
		return false;
	}

	esp_err_t err = i2c_master_write_read_device(
		IMU_I2C_PORT,
		addr,
		&reg,
		1,
		value,
		1,
		pdMS_TO_TICKS(50));

	return err == ESP_OK;
}

static void imu_i2c_diagnose(void)
{
	uint8_t whoami = 0;
	const uint8_t candidates[] = {0x68, 0x69};

	printf("I2C diagnose: SDA=GPIO%d SCL=GPIO%d @ %dHz\n",
	       CONFIG_IMU_I2C_SDA_GPIO,
	       CONFIG_IMU_I2C_SCL_GPIO,
	       IMU_I2C_FREQ_HZ);

	for (size_t i = 0; i < sizeof(candidates); ++i) {
		uint8_t addr = candidates[i];
		bool present = imu_i2c_probe_addr(addr);
		if (!present) {
			printf("I2C probe 0x%02X: no ACK\n", addr);
			continue;
		}

		printf("I2C probe 0x%02X: ACK\n", addr);
		if (imu_i2c_read_reg(addr, MPU6050_WHO_AM_I_REG, &whoami)) {
			printf("I2C 0x%02X WHO_AM_I(0x75)=0x%02X\n", addr, whoami);
		} else {
			printf("I2C 0x%02X WHO_AM_I read failed\n", addr);
		}
	}
}

static void imu_i2c_init(void)
{
	i2c_config_t conf = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = CONFIG_IMU_I2C_SDA_GPIO,
		.scl_io_num = CONFIG_IMU_I2C_SCL_GPIO,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = IMU_I2C_FREQ_HZ,
	};
	i2c_param_config(IMU_I2C_PORT, &conf);
	i2c_driver_install(IMU_I2C_PORT, conf.mode, 0, 0, 0);
}

static void imu_udp_task(void *pvParameters)
{
	(void)pvParameters;
	int16_t ax = 0, ay = 0, az = 0;
	int16_t gx = 0, gy = 0, gz = 0;
	char imu_msg[256];

	while (1) {
		if (init_ok) {
			uint64_t ts_us = esp_timer_get_time();
			mpu_getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
			snprintf(imu_msg, sizeof(imu_msg), "TS:%llu AX:%d,AY:%d,AZ:%d,GX:%d,GY:%d,GZ:%d\n",
			         ts_us, ax, ay, az, gx, gy, gz);
			printf("IMU: %s", imu_msg);
			udp_write_IMU((uint8_t *)imu_msg, (uint8_t)strlen(imu_msg));
		}
		vTaskDelay(pdMS_TO_TICKS(20));
	}
}

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

	/* 第2步: 初始化I2C并准备MPU6050 */
	printf("=== Initializing I2C for MPU6050 ===\n");
	imu_i2c_init();
	imu_i2c_diagnose();
	mpu_init();
	if (mpu_testConnection()) {
		printf("✓ MPU6050 connected\n\n");
	} else {
		printf("✗ MPU6050 not detected\n\n");
	}

	/* 第3步: 初始化UART1用于LD14激光雷达 */
	printf("=== Initializing UART1 (LD14 sensor) ===\n");
	UART_init();
	printf("✓ UART1 ready (230400 baud, GPIO17/GPIO18)\n\n");

	/* 第4步: 初始化UDP/TCP通信 */
	printf("=== Initializing UDP/TCP communication ===\n");
	UDP_init();
	printf("✓ UDP socket created\n");
	printf("✓ IMU UDP on port 8889\n");
	printf("✓ TCP monitor task started (heartbeat to 192.168.43.250:8888)\n\n");

	xTaskCreate(imu_udp_task, "imu_udp_task", 4096, NULL, 8, NULL);

	/* 第5步: 设置初始化完成标志 (允许传感器任务开始处理数据) */
	init_ok = true;
	printf("=== init_ok = true (gate opened for sensor tasks) ===\n\n");

	/* 第6步: 初始化LD14传感器 */
	printf("=== Initializing LD14 sensor ===\n");
	LD14_init();
	printf("✓ LD14_data_Task created and running\n\n");

	printf("=== System startup complete ===\n");
	printf("Data flow: LD14 → UART1 → CRC validate → UDP forward → PC\n");
	printf("TCP connection: monitoring heartbeat\n");
}
