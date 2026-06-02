#include "mpu_wrapper.h"

#ifdef HAVE_MPU6050_IMPL
#include "MPU6050.h"

#include <stdio.h>

static MPU6050 mpu_68(MPU6050_ADDRESS_AD0_LOW);
static MPU6050 mpu_69(MPU6050_ADDRESS_AD0_HIGH);
static MPU6050* active_mpu = NULL;
static bool mpu_ready = false;

extern "C" void mpu_init(void) {
    mpu_ready = false;
    active_mpu = NULL;

    // Try default address first (AD0 low), then fallback to AD0 high.
    mpu_68.initialize();
    if (mpu_68.testConnection()) {
        active_mpu = &mpu_68;
        mpu_ready = true;
        printf("MPU6050 connected at I2C address 0x68\n");
        return;
    }

    mpu_69.initialize();
    if (mpu_69.testConnection()) {
        active_mpu = &mpu_69;
        mpu_ready = true;
        printf("MPU6050 connected at I2C address 0x69\n");
        return;
    }

    printf("MPU6050 not found on I2C addresses 0x68/0x69\n");
}

extern "C" bool mpu_testConnection(void) {
    if (!mpu_ready || active_mpu == NULL) {
        return false;
    }
    return active_mpu->testConnection();
}

extern "C" void mpu_getMotion6(int16_t* ax, int16_t* ay, int16_t* az,
                                int16_t* gx, int16_t* gy, int16_t* gz) {
    if (!mpu_ready || active_mpu == NULL) {
        if (ax) *ax = 0;
        if (ay) *ay = 0;
        if (az) *az = 0;
        if (gx) *gx = 0;
        if (gy) *gy = 0;
        if (gz) *gz = 0;
        return;
    }
    active_mpu->getMotion6(ax, ay, az, gx, gy, gz);
}

extern "C" void mpu_getAcceleration(int16_t* x, int16_t* y, int16_t* z) {
    if (!mpu_ready || active_mpu == NULL) {
        if (x) *x = 0;
        if (y) *y = 0;
        if (z) *z = 0;
        return;
    }
    active_mpu->getAcceleration(x, y, z);
}

#else // stub implementations when MPU6050 sources are not present

#include <string.h>

static bool _mpu_inited = false;

extern "C" void mpu_init(void) {
    _mpu_inited = true;
}

extern "C" bool mpu_testConnection(void) {
    return false; // real implementation requires MPU6050 sources
}

extern "C" void mpu_getMotion6(int16_t* ax, int16_t* ay, int16_t* az,
                                int16_t* gx, int16_t* gy, int16_t* gz) {
    if (ax) *ax = 0;
    if (ay) *ay = 0;
    if (az) *az = 0;
    if (gx) *gx = 0;
    if (gy) *gy = 0;
    if (gz) *gz = 0;
}

extern "C" void mpu_getAcceleration(int16_t* x, int16_t* y, int16_t* z) {
    if (x) *x = 0;
    if (y) *y = 0;
    if (z) *z = 0;
}

#endif
