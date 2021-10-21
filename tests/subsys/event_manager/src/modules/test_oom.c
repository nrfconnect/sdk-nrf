/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <ztest.h>

#include <test_events.h>
#include <data_event.h>

#define MODULE test_oom
#define TEST_EVENTS_CNT 150

static struct data_event *event_tab[TEST_EVENTS_CNT];

/* Custom reboot handler to check if sys_reboot is called when OOM. */
void sys_reboot(int type)
{
	int i = 0;

	/* Freeing memory to enable further testing. */
	while (event_tab[i] != NULL) {
		k_free(event_tab[i]);
		i++;
	}

	ztest_test_pass();

	while (true) {
		;
	}
}

void test_oom_reset(void)
{
	/* Sending large number of events to cause out of memory error. */
	int i;

	for (i = 0; i < ARRAY_SIZE(event_tab); i++) {
		event_tab[i] = new_data_event();
		if (event_tab[i] == NULL) {
			break;
		}
	}

	/* This shall only be executed if OOM is not triggered. */
	zassert_true(i < ARRAY_SIZE(event_tab),
		     "No OOM detected, increase TEST_EVENTS_CNT");
	zassert_unreachable("OOM error not detected");
}
