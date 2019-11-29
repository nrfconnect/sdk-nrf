#ifndef DRIVER_VALIDATION_GEN_H
#define DRIVER_VALIDATION_GEN_H
#define Z_SYSCALL_DRIVER_GEN(ptr, op, driver_lower_case, driver_upper_case) \
		(Z_SYSCALL_OBJ(ptr, K_OBJ_DRIVER_##driver_upper_case) || \
		 Z_SYSCALL_DRIVER_OP(ptr, driver_lower_case##_driver_api, op))
                
#define Z_SYSCALL_DRIVER_ADC(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, adc, ADC)

#define Z_SYSCALL_DRIVER_AIO_CMP(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, aio_cmp, AIO_CMP)

#define Z_SYSCALL_DRIVER_COUNTER(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, counter, COUNTER)

#define Z_SYSCALL_DRIVER_CRYPTO(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, crypto, CRYPTO)

#define Z_SYSCALL_DRIVER_DMA(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, dma, DMA)

#define Z_SYSCALL_DRIVER_FLASH(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, flash, FLASH)

#define Z_SYSCALL_DRIVER_GPIO(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, gpio, GPIO)

#define Z_SYSCALL_DRIVER_I2C(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, i2c, I2C)

#define Z_SYSCALL_DRIVER_I2S(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, i2s, I2S)

#define Z_SYSCALL_DRIVER_IPM(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, ipm, IPM)

#define Z_SYSCALL_DRIVER_LED(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, led, LED)

#define Z_SYSCALL_DRIVER_PINMUX(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, pinmux, PINMUX)

#define Z_SYSCALL_DRIVER_PWM(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, pwm, PWM)

#define Z_SYSCALL_DRIVER_ENTROPY(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, entropy, ENTROPY)

#define Z_SYSCALL_DRIVER_SENSOR(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, sensor, SENSOR)

#define Z_SYSCALL_DRIVER_SPI(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, spi, SPI)

#define Z_SYSCALL_DRIVER_UART(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, uart, UART)

#define Z_SYSCALL_DRIVER_CAN(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, can, CAN)

#define Z_SYSCALL_DRIVER_PTP_CLOCK(ptr, op) Z_SYSCALL_DRIVER_GEN(ptr, op, ptp_clock, PTP_CLOCK)
#endif /* DRIVER_VALIDATION_GEN_H */
