#只有在这里写出的文件会被编译器编译和被其它地方访问
#这个文件修改一次需要重新编译所有文件 
#在这个文件里 # 号是注释  

set(CMAKE_CXX_STANDARD 11)

idf_component_register(
SRCS            #这里标记出目录下的c文件 
"LD14/LD14.c"
"MPU6050/I2Cdev.cpp"
"MPU6050/MPU6050.cpp"
"MPU6050/mpu_wrapper.cpp"
			
INCLUDE_DIRS  	#这里标记出目录中存放h文件的文件夹
"LD14/include"
"MPU6050"

REQUIRES        #标记出需要的资源
inside_communication 
Data_declaration
External_communication
esp_timer
driver

PRIV_REQUIRES   #标记出需要的资源


)

target_compile_definitions(${COMPONENT_LIB} PRIVATE HAVE_MPU6050_IMPL)
