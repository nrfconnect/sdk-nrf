/* Copyright (c) 2024, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "unity.h"

#include "cexception_assert.h"

#include "mock_vendor_radio_power_map.h"
#include "mock_vendor_radio_power_settings.h"

#include <stdint.h>
#include <stdio.h>

#include "vendor_radio_default_configuration.h"
#include "vendor_radio_power_limit.h"
#include "vendor_radio_power_limit.c"

void setUp(void)
{
}

void tearDown(void)
{
}

static void reset_plt(void)
{
	// power limits allow only one initialization per run. Make fake reset.
	m_selected_table = NULL;
	m_table_active_id = DEFAULT_ACTIVE_ID;
}

static void power_limit_init(int16_t plt, uint8_t region)
{
	reset_plt();
	vendor_radio_power_limit_id_read_ExpectAnyArgsAndReturn(OT_ERROR_NONE);
	vendor_radio_power_limit_id_read_ReturnThruPtr_p_id(&region);
	vendor_radio_power_limit_init(plt);
}

void test_power_limit_proper_init(void)
{
	uint8_t region_set;
	char version[10];
	uint8_t power_table_count;
	uint8_t region_index = 0;

	power_limit_init(PLT_ID_5, region_index);

	TEST_ASSERT_EQUAL(OT_ERROR_NONE, vendor_radio_power_limit_id_get(&region_set));
	TEST_ASSERT_EQUAL(region_index, region_set);
	TEST_ASSERT_EQUAL(OT_ERROR_NONE,
			  vendor_radio_power_limit_info_get(version, &power_table_count));
	TEST_ASSERT_EQUAL(m_table_5.header.power_table_count, power_table_count);
	TEST_ASSERT_EQUAL_STRING(m_table_5.header.power_table_version, version);
}

void test_power_limit_invalid_init(void)
{
	int16_t plt_index;
	uint8_t region_index;
	uint8_t region_set;
	char version[10];
	uint8_t power_table_count;

	// test invalid platform
	reset_plt();
	plt_index = 15;
	region_index = 1;
	vendor_radio_power_limit_id_read_ExpectAnyArgsAndReturn(OT_ERROR_NONE);
	vendor_radio_power_limit_id_read_ReturnThruPtr_p_id(&region_index);
	vendor_radio_power_limit_init(plt_index);

	TEST_ASSERT_EQUAL(OT_ERROR_NONE, vendor_radio_power_limit_id_get(&region_set));
	TEST_ASSERT_EQUAL(OT_ERROR_NONE,
			  vendor_radio_power_limit_info_get(version, &power_table_count));
	TEST_ASSERT_EQUAL(m_table_nrf_dk.header.power_table_count, power_table_count);
	TEST_ASSERT_EQUAL_STRING(m_table_nrf_dk.header.power_table_version, version);

	// test invalid region
	reset_plt();
	plt_index = PLT_ID_5;
	region_index = DEFAULT_ACTIVE_ID + 1;
	vendor_radio_power_limit_id_read_ExpectAnyArgsAndReturn(OT_ERROR_NOT_FOUND);
	vendor_radio_power_limit_id_read_ReturnThruPtr_p_id(&region_index);
	vendor_radio_power_limit_init(plt_index);
	TEST_ASSERT_EQUAL(OT_ERROR_NONE, vendor_radio_power_limit_id_get(&region_set));

	TEST_ASSERT_EQUAL(DEFAULT_ACTIVE_ID, region_set);
}

void test_power_limit_path_selection(void)
{
	uint8_t path;
	uint8_t region = 0;
	power_limit_table_data_t data;

	power_limit_init(PLT_ID_5, region);

	// path 0
	path = 0;
	vendor_radio_power_map_tx_path_id_get_ExpectAndReturn(path);
	TEST_ASSERT_EQUAL(OT_ERROR_NONE, vendor_radio_power_limit_get_active_table_limits(&data));
	TEST_ASSERT_EQUAL_MEMORY(m_table_5.data[region], &data, sizeof(power_limit_table_data_t));

	// path 1
	path = 1;
	vendor_radio_power_map_tx_path_id_get_ExpectAndReturn(path);
	TEST_ASSERT_EQUAL(OT_ERROR_NONE, vendor_radio_power_limit_get_active_table_limits(&data));
	TEST_ASSERT_EQUAL_MEMORY(m_table_5.data[region + m_table_5.header.power_table_count], &data,
				 sizeof(power_limit_table_data_t));

	// switch region
	region = 1;
	vendor_radio_power_limit_id_write_Expect(region);
	vendor_radio_power_limit_id_set(region);

	// path 0, different region
	path = 0;
	vendor_radio_power_map_tx_path_id_get_ExpectAndReturn(path);
	TEST_ASSERT_EQUAL(OT_ERROR_NONE, vendor_radio_power_limit_get_active_table_limits(&data));
	TEST_ASSERT_EQUAL_MEMORY(m_table_5.data[region], &data, sizeof(power_limit_table_data_t));

	// path 1, different region
	path = 1;
	vendor_radio_power_map_tx_path_id_get_ExpectAndReturn(path);
	TEST_ASSERT_EQUAL(OT_ERROR_NONE, vendor_radio_power_limit_get_active_table_limits(&data));
	TEST_ASSERT_EQUAL_MEMORY(m_table_5.data[region + m_table_5.header.power_table_count], &data,
				 sizeof(power_limit_table_data_t));
}

void test_power_limit_invalid_path_selection(void)
{
	uint8_t path;
	uint8_t region = 0;
	power_limit_table_data_t data;

	// multi-path limit
	power_limit_init(PLT_ID_5, region);

	// path 0
	path = 0;
	vendor_radio_power_map_tx_path_id_get_ExpectAndReturn(path);
	TEST_ASSERT_EQUAL(OT_ERROR_NONE, vendor_radio_power_limit_get_active_table_limits(&data));
	TEST_ASSERT_EQUAL_MEMORY(m_table_5.data[region], &data, sizeof(power_limit_table_data_t));

	// path 3
	path = 3;
	vendor_radio_power_map_tx_path_id_get_ExpectAndReturn(path);
	TEST_ASSERT_EQUAL(OT_ERROR_NOT_FOUND,
			  vendor_radio_power_limit_get_active_table_limits(&data));
}

void test_multipath_power_limiting(void)
{
	const uint8_t valid_region = 0;
	const uint8_t invalid_region = 99;
	uint8_t region;
	uint8_t channel;
	int8_t power;

	region = valid_region;
	power_limit_init(PLT_ID_5, region);

	for (channel = 11; channel <= 26; channel++) {
		TEST_ASSERT_EQUAL(OT_ERROR_NONE, vendor_radio_power_limit_get(channel, 0, &power));
		TEST_ASSERT_EQUAL(m_table_5.data[region][channel - 11], power);

		TEST_ASSERT_EQUAL(OT_ERROR_NONE, vendor_radio_power_limit_get(channel, 1, &power));
		TEST_ASSERT_EQUAL(
			m_table_5.data[region + m_table_5.header.power_table_count][channel - 11],
			power);
	}

	power_limit_init(PLT_ID_5, invalid_region);
	TEST_ASSERT_EQUAL(OT_ERROR_NONE, vendor_radio_power_limit_get(11, 0, &power));
	TEST_ASSERT_EQUAL(DEFAULT_TX_POWER_LIMIT, power);
}

void test_power_limiting_errors(void)
{
	uint8_t region = 0;
	int8_t power;

	power_limit_init(PLT_ID_5, region);

	TEST_ASSERT_EQUAL(OT_ERROR_INVALID_ARGS, vendor_radio_power_limit_get(10, 0, &power));
	TEST_ASSERT_EQUAL(OT_ERROR_INVALID_ARGS, vendor_radio_power_limit_get(27, 0, &power));

	TEST_ASSERT_EQUAL(OT_ERROR_INVALID_ARGS, vendor_radio_power_limit_get(10, 1, &power));
	TEST_ASSERT_EQUAL(OT_ERROR_INVALID_ARGS, vendor_radio_power_limit_get(27, 1, &power));

	TEST_ASSERT_EQUAL(OT_ERROR_INVALID_ARGS, vendor_radio_power_limit_get(11, 2, &power));
	TEST_ASSERT_EQUAL(OT_ERROR_INVALID_ARGS, vendor_radio_power_limit_get(26, 2, &power));
}