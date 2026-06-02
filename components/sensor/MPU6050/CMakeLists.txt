set(CMAKE_CXX_STANDARD 11)

# Register the MPU6050 component with ESP-IDF-specific I2Cdev implementation.
# Sources included: I2Cdev (ESP-IDF), MPU6050 (original Arduino-derived), wrapper.
idf_component_register(SRCS "I2Cdev.cpp" "MPU6050.cpp" "mpu_wrapper.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES driver)

# Export compile definitions for the wrapper and to include the MotionApps DMP blob
target_compile_definitions(${COMPONENT_LIB} PRIVATE HAVE_MPU6050_IMPL MPU6050_INCLUDE_DMP_MOTIONAPPS20)
