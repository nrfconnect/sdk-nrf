/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>

#include "modules_common.h"

extern int unity_main(void);
extern int generic_suiteTearDown(int num_failures);

/* Suite teardown shall finalize with mandatory call to generic_suiteTearDown. */
int test_suiteTearDown(int num_failures)
{
	return generic_suiteTearDown(num_failures);
}

void setUp(void)
{

}

void tearDown(void)
{

}

void test_state_set(void)
{

}

void main(void)
{
	(void)unity_main();
}
