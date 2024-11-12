/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/devicetree.h>
#include <hal/nrf_vpr_csr.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(power_off, LOG_LEVEL_INF);

#define VIPER_SLEEP_SUBSTATE_WAIT 0
#define VIPER_SLEEP_SUBSTATE_SLEEP 1
#define VIPER_SLEEP_SUBSTATE_HIBERNATE 0
#define VIPER_SLEEP_SUBSTATE_DEEPSLEEP 1

static void pm_go_to_wait(void)
{
	LOG_INF("Wait Sleep State");

	csr_write(VPRCSR_NORDIC_VPRNORDICSLEEPCTRL,
		  VPRCSR_NORDIC_VPRNORDICSLEEPCTRL_SLEEPSTATE_WAIT);
	nrf_barrier_w();
	arch_cpu_idle();
}

static void pm_go_to_hibernate(void)
{
	LOG_INF("Hibernate Sleep State");

	csr_write(VPRCSR_NORDIC_VPRNORDICSLEEPCTRL,
		  VPRCSR_NORDIC_VPRNORDICSLEEPCTRL_SLEEPSTATE_HIBERNATE);
	nrf_barrier_w();
	arch_cpu_idle();
}

/**
 * @brief Zephyr callback function to set/enter new PM State
 *
 * @param state Current PM State
 * @param substate_id Substate of current PM State
 */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	LOG_INF("PPR pm_state_set: %d\t%d", (int)state, (int)substate_id);

	switch (state) {
	case PM_STATE_SUSPEND_TO_RAM: {
		switch (substate_id) {
		case VIPER_SLEEP_SUBSTATE_HIBERNATE:
			pm_go_to_hibernate();
			break;
		case VIPER_SLEEP_SUBSTATE_DEEPSLEEP:
			/* do nothing */
			break;
		}
	} break;
	case PM_STATE_STANDBY: {
		switch (substate_id) {
		case VIPER_SLEEP_SUBSTATE_WAIT:
			pm_go_to_wait();
			break;
		}
	} break;
	case PM_STATE_SUSPEND_TO_IDLE:
	case PM_STATE_SUSPEND_TO_DISK:
	case PM_STATE_SOFT_OFF:
	case PM_STATE_COUNT:
	case PM_STATE_ACTIVE:
	case PM_STATE_RUNTIME_IDLE:
		break;
	}
}

/**
 * @brief Zephyr callback function to exit current PM State
 *
 * @param state Current PM State
 * @param substate_id Substate of current PM State
 *
 */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	LOG_INF("PPR pm_state_exit_post_ops");
	ARG_UNUSED(substate_id);

	csr_write(VPRCSR_NORDIC_VPRNORDICSLEEPCTRL,
		  VPRCSR_NORDIC_VPRNORDICSLEEPCTRL_SLEEPSTATE_WAIT);

	/* unlock interrupts after sleep */
	irq_unlock(MSTATUS_IEN);
}
