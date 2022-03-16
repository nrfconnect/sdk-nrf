#ifndef CPU_UTILIZATION_H__
#define CPU_UTILIZATION_H__

#include <stdint.h>

#define BSP_BOARD_LED_2 2


int cpu_utilization_start();

int cpu_utilization_stop();

void cpu_utilization_clear();

uint32_t cpu_utilization_get();

void bsp_board_leds_off(void);

typedef uint32_t nrf_cli_t;

#endif /* CPU_UTILIZATION_H__ */
