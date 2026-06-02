MPU6050 component wrapper for ESP-IDF

This component exposes a small C API for use from C code: `mpu_init`,
`mpu_testConnection`, `mpu_getMotion6`, `mpu_getAcceleration`.

Integration steps:
1. Copy the original Arduino `I2Cdev.cpp/.h` and `MPU6050.cpp/.h` files
   into this folder (`components/MPU6050/`).
2. Update `CMakeLists.txt` to include those sources, e.g.:

   idf_component_register(SRCS "I2Cdev.cpp" "MPU6050.cpp" "mpu_wrapper.cpp" INCLUDE_DIRS ".")

3. Define `HAVE_MPU6050_IMPL` so the wrapper will call the real class
   implementation. You can add the definition in the component's
   `CMakeLists.txt` with `target_compile_definitions` or by adding the
   macro to your global build flags.

Notes and caveats:
- The copied Arduino sources depend on `Wire`, `Serial`, and `millis()`.
  You will likely need to port `I2Cdev` to use ESP-IDF I2C APIs or provide
  small Arduino compatibility shims.
- Alternatively, implement low-level read/write using ESP-IDF I2C and call
  the MPU6050 register-level functions directly.
