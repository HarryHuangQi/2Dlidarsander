#只有在这里写出的文件会被编译器编译和被其它地方访问
#这个文件修改一次需要重新编译所有文件
#在这个文件里 # 号是注释

idf_component_register(
SRCS            #这里标记出目录下的c文件
"UART/UART.c"
			
INCLUDE_DIRS  	#这里标记出目录中存放h文件的文件夹
"UART/include"

REQUIRES        #标记出需要的资源
Data_declaration
freertos
esp_system
hal
soc
esp_driver_uart
esp_driver_gpio
PRIV_REQUIRES   #标记出需要的资源
esp_hal_uart

)