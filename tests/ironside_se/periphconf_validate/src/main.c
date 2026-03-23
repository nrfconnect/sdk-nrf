/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <ironside_zephyr/se/uicr_periphconf.h>

#define IS_APP IS_ENABLED(CONFIG_SOC_NRF54H20_CPUAPP)
#define IS_RAD IS_ENABLED(CONFIG_SOC_NRF54H20_CPURAD)

/* Fine: shared entries should not conflict as they are identical. */
UICR_PERIPHCONF_ENTRY(PERIPHCONF_GPIO_PIN_CNF_CTRLSEL(DT_REG_ADDR(DT_NODELABEL(gpio0)), 0, 0));
UICR_PERIPHCONF_ENTRY(PERIPHCONF_GPIO_PIN_CNF_CTRLSEL(DT_REG_ADDR(DT_NODELABEL(gpio0)), 1, 0));

/* Error: conflict in non-lockable regs.
 * both application and radio attempt to get the ADC interrupt.
 */
UICR_PERIPHCONF_ENTRY(PERIPHCONF_IRQMAP_IRQ_SINK(DT_IRQN_BY_IDX(DT_NODELABEL(adc), 0),
						 NRF_PROCESSOR));

/* Error: conflict in lockable regs.
 * both application and radio take ownership of the same pins
 */
UICR_PERIPHCONF_ENTRY(PERIPHCONF_SPU_FEATURE_GPIO_PIN(0x5f920000,
						      DT_PROP(DT_NODELABEL(gpio0), port), 0, true,
						      NRF_OWNER, true));
UICR_PERIPHCONF_ENTRY(PERIPHCONF_SPU_FEATURE_GPIO_PIN(0x5f920000,
						      DT_PROP(DT_NODELABEL(gpio0), port), 0, true,
						      NRF_OWNER, true));

#if IS_APP
/* Error: configuring unimplemented DPPI channels. */
UICR_PERIPHCONF_ENTRY(PERIPHCONF_SPU_FEATURE_DPPIC_CH(0x5f990000, DPPIC132_CH_NUM_SIZE, true,
						      NRF_OWNER, true));
UICR_PERIPHCONF_ENTRY(PERIPHCONF_SPU_FEATURE_DPPIC_CH(0x5f990000, DPPIC132_CH_NUM_SIZE + 1, false,
						      NRF_OWNER, true));

/* Error: configuring OOB IPCMAP channel index, will be considered 'unrecognized'. */
UICR_PERIPHCONF_ENTRY(PERIPHCONF_IPCMAP_CHANNEL_SINK(16, NRF_DOMAIN_APPLICATION, 0));
UICR_PERIPHCONF_ENTRY(PERIPHCONF_IPCMAP_CHANNEL_SOURCE(16, NRF_DOMAIN_GLOBAL, 0, true));

/* Error: Setting DMASEC=1 for a peripheral without DMA (TIMER120). */
UICR_PERIPHCONF_ENTRY(PERIPHCONF_SPU_PERIPH_PERM(0x5f8e0000UL, 2, true, true, NRF_OWNER_APPLICATION,
						 true));

#endif
/* TODO: switch ownerid on periph w/o ownerprog */
/* TODO: switch secattr on a periph w/o programmable secattr. */
/* TODO: modify locked SPU PERM register (are there any?). */
/* TODO: modify locked SPU FEATURE register (are there any?) */
/* TODO: modify bad MEMCONF region (need macros first) */
