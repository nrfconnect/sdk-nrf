/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spim_pm_sck_cs, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <soc.h>
#include <hal/nrf_gpio.h>

#define USER_NODE DT_PATH(zephyr_user)

#define SCK_INPUT_PIN NRF_DT_GPIOS_TO_PSEL(USER_NODE, spim_sck_gpios)
#define CS_INPUT_PIN  NRF_DT_GPIOS_TO_PSEL(USER_NODE, spim_cs_gpios)

BUILD_ASSERT((SCK_INPUT_PIN >> 5) == (CS_INPUT_PIN >> 5),
	     "SCK and CS inputs must be on the same GPIO port");

#define CS_ACTIVE_LEVEL ((DT_GPIO_FLAGS(USER_NODE, spim_cs_gpios) & GPIO_ACTIVE_LOW) ? 0 : 1)

/* CPOL = 1 for SPI modes 2 and 3. */
#define SCK_IDLE_LEVEL ((CONFIG_TEST_SPI_MODE >= 2) ? 1 : 0)

/* One byte transmitted: 8 clock periods, both edges counted. */
#define SCK_EDGES_PER_BYTE 16

#if defined(CONFIG_TEST_SCENARIO_RTIO_QUEUED) || defined(CONFIG_TEST_SCENARIO_RTIO_CALLBACK)
#define CS_WINDOWS_EXPECTED  2
#define SCK_EDGES_PER_WINDOW SCK_EDGES_PER_BYTE
#elif defined(CONFIG_TEST_SCENARIO_RTIO_HOLD_ON_CS)
#define CS_WINDOWS_EXPECTED  1
#define SCK_EDGES_PER_WINDOW (2 * SCK_EDGES_PER_BYTE)
#else
#define CS_WINDOWS_EXPECTED  1
#define SCK_EDGES_PER_WINDOW SCK_EDGES_PER_BYTE
#endif

#if defined(CONFIG_TEST_SCENARIO_RTIO_QUEUED)
#define SCENARIO_NAME "rtio_queued"
#elif defined(CONFIG_TEST_SCENARIO_RTIO_CALLBACK)
#define SCENARIO_NAME "rtio_callback"
#elif defined(CONFIG_TEST_SCENARIO_RTIO_HOLD_ON_CS)
#define SCENARIO_NAME "rtio_hold_on_cs"
#else
#define SCENARIO_NAME "sync"
#endif

#define TRANSITIONS_MAX		   256
#define CS_WINDOWS_MAX		   8
#define SCK_EDGES_OUTSIDE_REPORTED 4

/* Covers the application core start-up and all transfers. */
#define CAPTURE_TIME_MS 1000

/* Number of unchanged samples between timeout checks. It must be large enough
 * to never check the time (slow GRTC access) while SPI lines toggle.
 */
#define IDLE_SAMPLES_PER_TIME_CHECK 4096

struct transition {
	uint32_t sample;
	uint8_t sck;
	uint8_t cs;
};

static struct transition transitions[TRANSITIONS_MAX];

/* SPI lines are sampled in a tight loop with interrupts disabled, so that
 * no edge is lost, even if CS edges are only a few microseconds apart.
 */
static uint32_t lines_capture(NRF_GPIO_Type *port, uint32_t sck_mask, uint32_t cs_mask,
			      uint32_t *initial_state)
{
	uint32_t mask = sck_mask | cs_mask;
	uint32_t start = k_cycle_get_32();
	uint32_t capture_time = k_ms_to_cyc_ceil32(CAPTURE_TIME_MS);
	uint32_t idle_samples = 0;
	uint32_t sample = 0;
	uint32_t count = 0;
	uint32_t prev;
	uint32_t state;
	unsigned int key;

	key = irq_lock();

	prev = nrf_gpio_port_in_read(port) & mask;
	*initial_state = prev;

	while (true) {
		state = nrf_gpio_port_in_read(port) & mask;
		sample++;

		if (state != prev) {
			if (count < TRANSITIONS_MAX) {
				transitions[count].sample = sample;
				transitions[count].sck = (state & sck_mask) ? 1 : 0;
				transitions[count].cs = (state & cs_mask) ? 1 : 0;
			}
			count++;
			prev = state;
			idle_samples = 0;
			continue;
		}

		if ((++idle_samples % IDLE_SAMPLES_PER_TIME_CHECK) != 0) {
			continue;
		}

		if ((k_cycle_get_32() - start) > capture_time) {
			break;
		}
	}

	irq_unlock(key);

	return count;
}

static bool transitions_analyze(uint32_t sck, uint32_t cs, uint32_t count)
{
	uint32_t sck_edges[CS_WINDOWS_MAX] = {0};
	uint32_t sck_edges_outside = 0;
	uint32_t cs_inactive_since = 0;
	uint32_t windows = 0;
	bool passed = true;

	if (sck != SCK_IDLE_LEVEL) {
		LOG_WRN("SCK is not in the idle state before the transfer");
	}

	if (cs == CS_ACTIVE_LEVEL) {
		LOG_ERR("CS is active before the transfer");
		passed = false;
	}

	for (uint32_t i = 0; i < MIN(count, TRANSITIONS_MAX); i++) {
		const struct transition *t = &transitions[i];
		bool sck_changed = (t->sck != sck);
		bool cs_changed = (t->cs != cs);

		if (cs_changed) {
			bool activation = (t->cs == CS_ACTIVE_LEVEL);

			if (activation) {
				windows++;
				LOG_INF("CS activation %u at sample %u (CS inactive for %u "
					"samples)",
					windows, t->sample, t->sample - cs_inactive_since);
			} else {
				cs_inactive_since = t->sample;
				LOG_INF("CS deactivation %u at sample %u, SCK edges in window: %u",
					windows, t->sample,
					(windows > 0) && (windows <= CS_WINDOWS_MAX)
						? sck_edges[windows - 1]
						: 0);
			}

			if (sck_changed) {
				LOG_ERR("SCK and CS changed in the same sample at CS %s %u",
					activation ? "activation" : "deactivation", windows);
				passed = false;
			} else if (sck != SCK_IDLE_LEVEL) {
				LOG_ERR("SCK is not in the idle state at CS %s %u",
					activation ? "activation" : "deactivation", windows);
				passed = false;
			}
		}

		if (sck_changed) {
			if (t->cs == CS_ACTIVE_LEVEL) {
				if ((windows > 0) && (windows <= CS_WINDOWS_MAX)) {
					sck_edges[windows - 1]++;
				}
			} else {
				if (sck_edges_outside < SCK_EDGES_OUTSIDE_REPORTED) {
					LOG_INF("SCK edge to %u while CS inactive at sample %u",
						t->sck, t->sample);
				}
				sck_edges_outside++;
			}
		}

		sck = t->sck;
		cs = t->cs;
	}

	if (count > TRANSITIONS_MAX) {
		LOG_ERR("%u transitions captured, only %u analyzed", count, TRANSITIONS_MAX);
		passed = false;
	}

	if (cs == CS_ACTIVE_LEVEL) {
		LOG_ERR("CS is still active after the transfer");
		passed = false;
	}

	LOG_INF("CS windows: %u, SCK edges outside CS windows: %u", windows, sck_edges_outside);

	if (windows != CS_WINDOWS_EXPECTED) {
		LOG_ERR("Found %u CS windows, %u expected", windows, CS_WINDOWS_EXPECTED);
		passed = false;
	}

	for (uint32_t w = 0; w < MIN(windows, CS_WINDOWS_MAX); w++) {
		if (sck_edges[w] != SCK_EDGES_PER_WINDOW) {
			LOG_ERR("Found %u SCK edges in CS window %u, %u expected", sck_edges[w],
				w + 1, SCK_EDGES_PER_WINDOW);
			passed = false;
		}
	}

	if (sck_edges_outside != 0) {
		LOG_WRN("Found %u SCK edges while CS was inactive", sck_edges_outside);
	}

	return passed;
}

int main(void)
{
	uint32_t sck_pin = SCK_INPUT_PIN;
	uint32_t cs_pin = CS_INPUT_PIN;
	NRF_GPIO_Type *port = nrf_gpio_pin_port_decode(&sck_pin);
	uint32_t initial_state;
	uint32_t count;
	bool passed;

	(void)nrf_gpio_pin_port_decode(&cs_pin);

	LOG_INF("SPIM PM SCK CS test on %s", CONFIG_BOARD_TARGET);
	LOG_INF("Scenario: %s, SPI mode: %d, SCK idle level: %d", SCENARIO_NAME,
		CONFIG_TEST_SPI_MODE, SCK_IDLE_LEVEL);
	LOG_INF("Expect %d CS window(s) with %d SCK edges each", CS_WINDOWS_EXPECTED,
		SCK_EDGES_PER_WINDOW);
	LOG_INF("===================================================================");

	nrf_gpio_cfg_input(SCK_INPUT_PIN, NRF_GPIO_PIN_NOPULL);
	nrf_gpio_cfg_input(CS_INPUT_PIN, NRF_GPIO_PIN_NOPULL);

	count = lines_capture(port, BIT(sck_pin), BIT(cs_pin), &initial_state);

	LOG_INF("Captured %u transitions", count);

	if (count == 0) {
		LOG_ERR("Timeout, no activity on SPI lines");
		passed = false;
	} else {
		passed = transitions_analyze((initial_state & BIT(sck_pin)) ? 1 : 0,
					     (initial_state & BIT(cs_pin)) ? 1 : 0, count);
	}

	LOG_INF("Test %s", passed ? "passed" : "failed");

	return 0;
}
