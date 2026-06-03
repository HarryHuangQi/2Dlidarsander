# lidarsender

一个基于 ESP-IDF 的车载传感器网关项目，负责把激光雷达、IMU 和网络通信整合到同一套固件里。

## 项目功能

- Wi-Fi STA 连接，默认使用静态 IP `192.168.43.105`
- I2C 初始化 MPU6050，并定时读取 IMU 数据
- UART1 接入 LD14 激光雷达并转发原始数据
- 通过 UDP 发送激光雷达数据与 IMU 数据
- 通过 TCP 发送心跳包，监测上位机连通状态

## 默认工作流程

程序启动后会按下面的顺序初始化：

1. Wi-Fi
2. MPU6050 的 I2C 总线
3. UART1 和 LD14
4. UDP / TCP 通信
5. 开启 IMU 数据任务

## 默认参数

- Wi-Fi 目标网络：通过 [components/External_communication/WIFI/WIFI.c](components/External_communication/WIFI/WIFI.c) 或工程配置项设置，仓库默认值已脱敏
- 默认静态 IP：`192.168.43.105`
- 默认网关：`192.168.43.1`
- 上位机地址：`10.42.0.1` 或代码中当前配置的目标地址
- LD14 转发端口：`8888`
- IMU UDP 端口：`8889`
- TCP 心跳端口：`8888`
- UART1 引脚：GPIO17 / GPIO18
- MPU6050 I2C 引脚：SDA GPIO4，SCL GPIO5

## 目录结构

- [main/](main/)：应用入口
- [components/](components/)：业务组件
- [espressif__ethernet_init/](espressif__ethernet_init/)：Espressif 以太网初始化组件
- [managed_components/](managed_components/)：受管依赖

## 构建与烧录

确保已经安装 ESP-IDF，然后在项目根目录执行：

```bash
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

如果你的板子不是 ESP32-S3，请先把 target 改成实际芯片型号。

## 配置说明

- Wi-Fi 账号和密码通过工程配置项或本地环境设置，仓库里只保留占位默认值
- 上位机 IP 和端口写在 [components/External_communication/UDP_TCP/UDP_TCP.c](components/External_communication/UDP_TCP/UDP_TCP.c)
- MPU6050 的 I2C 引脚在 [main/main.c](main/main.c) 中通过宏定义控制

## 开源协议

本项目采用 MIT License，属于非常宽松的开源协议，允许自由使用、修改、分发和商用，前提是保留版权和许可声明。

## 许可证

请查看 [LICENSE](LICENSE)。
