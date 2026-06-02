#ifndef MPU_WRAPPER_H
#define MPU_WRAPPER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize MPU (power on, basic config)
void mpu_init(void);

// Return true if device responds
bool mpu_testConnection(void);

// Read accel+gyro (raw int16 values)
void mpu_getMotion6(int16_t* ax, int16_t* ay, int16_t* az,
                    int16_t* gx, int16_t* gy, int16_t* gz);

// Convenience: read only accelerometer
void mpu_getAcceleration(int16_t* x, int16_t* y, int16_t* z);

#ifdef __cplusplus
}
#endif

#endif // MPU_WRAPPER_H
