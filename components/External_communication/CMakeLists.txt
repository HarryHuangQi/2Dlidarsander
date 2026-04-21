#只有在这里写出的文件会被编译器编译和被其它地方访问
#这个文件修改一次需要重新编译所有文件
#在这个文件里 # 号是注释

idf_component_register(
SRCS            #这里标记出目录下的c文件
 "WIFI/WIFI.c" "UDP_TCP/UDP_TCP.c"
INCLUDE_DIRS  	#这里标记出目录中存放h文件的文件夹
 "WIFI/include" "UDP_TCP/include"

REQUIRES        #标记出需要的资源
inside_communication 
nvs_flash
Data_declaration
sensor
esp_wifi
esp_event
lwip
esp_netif
freertos
PRIV_REQUIRES   #标记出需要的资源

)