/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/platform/hooks.h>
#include "nrf7120_enga_types.h"
#include "nrf7120_enga_global.h"

/* Wi-Fi core defines (not part of MDK) */
#define LRC0_POWERON    (0x4800B000UL + 0x00000490UL)
#define LMAC_VPR_INITPC (0x48000000UL + 0x00000808UL)
#define LMAC_VPR_CPURUN (0x48000000UL + 0x00000800UL)

static inline void write_reg32(uintptr_t addr, uint32_t value)
{
	*((volatile uint32_t *)addr) = value;
}

#if CONFIG_BOARD_EARLY_INIT_HOOK
/* Temporary workaround while VPR does not handle starting clocks */
void board_early_init_hook(void)
{
#if defined(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC)
	NRF_CLOCK_S->LFCLK.SRC = CLOCK_LFCLK_SRC_SRC_LFRC;
#elif DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lfxo))
	NRF_CLOCK_S->LFCLK.SRC = CLOCK_LFCLK_SRC_SRC_LFXO;
#else
	NRF_CLOCK_S->LFCLK.SRC = CLOCK_LFCLK_SRC_SRC_LFSYNT;
#endif
	NRF_CLOCK_S->TASKS_LFCLKSTART = 1;

	/* Wait for event */
	while (NRF_CLOCK_S->EVENTS_LFCLKSTARTED !=
	    CLOCK_EVENTS_LFCLKSTARTED_EVENTS_LFCLKSTARTED_Generated) {
		/* busy-wait */
	}

	/* Make entire RAM accessible to the Wi-Fi domain */
	NRF_MPC00->OVERRIDE[0].STARTADDR = 0x20000000;
	NRF_MPC00->OVERRIDE[0].ENDADDR = 0x20100000;
	NRF_MPC00->OVERRIDE[0].PERM = 0x7; /* 0-NS R,W,X =1 */
	NRF_MPC00->OVERRIDE[0].PERMMASK = 0xF;
	NRF_MPC00->OVERRIDE[0].CONFIG = 0x1200;

	/* MRAM MPC overrides for wifi */
	NRF_MPC00->OVERRIDE[1].STARTADDR = 0x00000000;
	NRF_MPC00->OVERRIDE[1].ENDADDR = 0x01000000;
	NRF_MPC00->OVERRIDE[1].PERM = 0x7;
	NRF_MPC00->OVERRIDE[1].PERMMASK = 0xF;
	NRF_MPC00->OVERRIDE[1].CONFIG = 0x1200;

	/* Make GRTC accessable from the WIFI-Core */
	NRF_SPU20->PERIPH[34].PERM = (SPU_PERIPH_PERM_SECATTR_NonSecure << SPU_PERIPH_PERM_SECATTR_Pos);

	/* Wi-Fi VPR uses UART 20 (PORT 2 Pin 2 is for the TX) */
#define UARTE20_ID ((NRF_UARTE20_S_BASE >> 12) & 0x1F)
	NRF_SPU20->PERIPH[UARTE20_ID].PERM =
		(SPU_PERIPH_PERM_SECATTR_NonSecure << SPU_PERIPH_PERM_SECATTR_Pos);
	NRF_SPU00->FEATURE.GPIO[2].PIN[2] =
		(SPU_FEATURE_GPIO_PIN_SECATTR_NonSecure << SPU_FEATURE_GPIO_PIN_SECATTR_Pos);
	/* Set permission for TXD */
	NRF_SPU20->FEATURE.GPIO[1].PIN[4] =
		(SPU_FEATURE_GPIO_PIN_SECATTR_NonSecure << SPU_FEATURE_GPIO_PIN_SECATTR_Pos);

	/* Split security configuration to let Wi-Fi access GRTC */
	NRF_SPU20->FEATURE.GRTC.CC[15] = 0;
	NRF_SPU20->FEATURE.GRTC.CC[14] = 0;
	NRF_SPU20->FEATURE.GRTC.INTERRUPT[4] = 0;
	NRF_SPU20->FEATURE.GRTC.INTERRUPT[5] = 0;
	NRF_SPU20->FEATURE.GRTC.SYSCOUNTER = 0;

	write_reg32(LRC0_POWERON,
		    (LRCCONF_POWERON_MAIN_AlwaysOn << LRCCONF_POWERON_MAIN_Pos));
	write_reg32(LMAC_VPR_INITPC, 0x28080000);
	write_reg32(LMAC_VPR_CPURUN,
		    (VPR_CPURUN_EN_Running << VPR_CPURUN_EN_Pos));
}
#endif
