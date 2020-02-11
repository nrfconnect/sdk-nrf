/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <init.h>
#include <device.h>
#include <zephyr.h>

#include <bsd.h>
#include <bsd_platform.h>

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) &&\
	!defined(CONFIG_BSD_LIBRARY_ALLOW_SECURE)
#error  bsdlib must be run as non-secure firmware. Are you building for the correct board ?
#endif

#ifdef CONFIG_BSD_LIBRARY_ALLOW_SECURE
#define SRAM_WRITE                                                             \
	((SPU_RAMREGION_PERM_WRITE_Enable << SPU_RAMREGION_PERM_WRITE_Pos) &   \
	 SPU_RAMREGION_PERM_WRITE_Msk)

#define SRAM_READ                                                              \
	((SPU_RAMREGION_PERM_READ_Enable << SPU_RAMREGION_PERM_READ_Pos) &     \
	 SPU_RAMREGION_PERM_READ_Msk)

#define SRAM_LOCK                                                              \
	((SPU_RAMREGION_PERM_LOCK_Locked << SPU_RAMREGION_PERM_LOCK_Pos) &     \
	 SPU_RAMREGION_PERM_LOCK_Msk)

#define SRAM_NONSEC                                                            \
	((SPU_RAMREGION_PERM_SECATTR_Non_Secure                                \
	  << SPU_RAMREGION_PERM_SECATTR_Pos) &                                 \
	 SPU_RAMREGION_PERM_SECATTR_Msk)

#define PERIPH_NONSEC                                                          \
	((SPU_PERIPHID_PERM_SECATTR_NonSecure                                  \
	  << SPU_PERIPHID_PERM_SECATTR_Pos) &                                  \
	 SPU_PERIPHID_PERM_SECATTR_Msk)

#define PERIPH_LOCK                                                            \
	((SPU_PERIPHID_PERM_LOCK_Locked << SPU_PERIPHID_PERM_LOCK_Pos) &       \
	 SPU_PERIPHID_PERM_LOCK_Msk)
#if defined(CONFIG_SOC_NRF9160)
#define SPU_RAMREGION_SIZE (256/32)
#define RAM_START_ADDR 0x20000000
#endif
static inline void setup_ns_ram(void)
{
	u8_t start_block = (BSD_RESERVED_MEMORY_ADDRESS - RAM_START_ADDR) /
			   SPU_RAMREGION_SIZE;
	u8_t size = BSD_RESERVED_MEMORY_SIZE / SPU_RAMREGION_SIZE;

	for (u8_t i = start_block; i < (start_block + size); i++) {
		NRF_SPU_S->RAMREGION[i].PERM = SRAM_READ | SRAM_WRITE |
					       SRAM_LOCK  | SRAM_NONSEC;
	}
}

static inline void setup_clock(void)
{
	NRF_CLOCK_S->LFCLKSRC = CLOCK_LFCLKSRCCOPY_SRC_LFRC
				<< CLOCK_LFCLKSRC_SRC_Pos;
	NRF_CLOCK_S->EVENTS_LFCLKSTARTED  = 0;
	NRF_CLOCK_S->TASKS_LFCLKSTART = 1;
	while (NRF_CLOCK_S->EVENTS_LFCLKSTARTED == 0) {
	};
	NRF_CLOCK_S->EVENTS_LFCLKSTARTED = 0;
}

static inline void setup_ns_power(void)
{
	NRF_SPU_S->PERIPHID[NRFX_PERIPHERAL_ID_GET(NRF_POWER_NS)].PERM =
	       PERIPH_LOCK | PERIPH_NONSEC;
	NRF_SPU_S->PERIPHID[NRFX_PERIPHERAL_ID_GET(NRF_POWER_S)].PERM = 0;
}

static inline void setup_s_ipc(void)
{
	NRF_SPU_S->PERIPHID[NRFX_PERIPHERAL_ID_GET(NRF_IPC_S)].PERM =
		PERIPH_LOCK | PERIPH_NONSEC;
}

static inline void setup_for_secure(void)
{
	setup_clock();
	setup_ns_power();
	setup_s_ipc();
	setup_ns_ram();
}
#endif

extern void ipc_proxy_irq_handler(void);

static int init_ret;

static int _bsdlib_init(struct device *unused)
{
	/* Setup the network IRQ used by the BSD library.
	 * Note: No call to irq_enable() here, that is done through bsd_init().
	 */
	IRQ_DIRECT_CONNECT(BSD_NETWORK_IRQ, BSD_NETWORK_IRQ_PRIORITY,
			   ipc_proxy_irq_handler, 0);

	init_ret = bsd_init();

	if (IS_ENABLED(CONFIG_BSD_LIBRARY_SYS_INIT)) {
		/* bsd_init() returns values from a different namespace
		 * than Zephyr's. Make sure to return something in Zephyr's
		 * namespace, in this case 0, when called during SYS_INIT.
		 * Non-zero values in SYS_INIT are currently ignored.
		 */
		return 0;
	}

	return init_ret;
}

int bsdlib_init(void)
{
#ifdef CONFIG_BSD_LIBRARY_ALLOW_SECURE
	setup_for_secure();
#endif
	return _bsdlib_init(NULL);
}

int bsdlib_get_init_ret(void)
{
	return init_ret;
}

int bsdlib_shutdown(void)
{
	bsd_shutdown();

	return 0;
}

#if defined(CONFIG_BSD_LIBRARY_SYS_INIT)
/* Initialize during SYS_INIT */
SYS_INIT(_bsdlib_init, POST_KERNEL, 0);
#endif
