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

/*
 * @file
 * @brief # This file provides an implementation of a Openthread CLI vendor extension.
 */

#include <stdlib.h>
#include <string.h>

#include <nrfx_temp.h>
#include <openthread/cli.h>
#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/link.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include "common/code_utils.hpp"
#include "utils/parse_cmdline.hpp"
#include "../models/power_map/vendor_radio_power_limit.h"
#include "../models/power_map/vendor_radio_power_map.h"
#include "../models/power_map/vendor_radio_power_map_psemi.h"
#include "../models/power_map/vendor_radio_power_settings.h"
#include "../models/power_map/utils.h"
#include "../models/power_map/crc32.h"

extern "C" {
#include <mpsl_fem_protocol_api.h>
#include <hal/nrf_radio.h>
#if defined(NRF54L_SERIES)
#include <haly/nrfy_grtc.h>
#endif
#include "../psemi/include/mpsl_fem_psemi_interface.h"
}

#ifdef CONFIG_THREAD_MONITOR
#define MAX_THREADS 16

struct thread_info
{
	char		name[CONFIG_THREAD_MAX_NAME_LEN];
	uintptr_t	stack;
	uint32_t	stack_size;
	uintptr_t	entry;
	int8_t		prio;
};

typedef struct
{
	uint32_t count;
	struct thread_info threads[MAX_THREADS];
} thread_processing_data;
#endif /* CONFIG_THREAD_MONITOR */

const uint8_t ATTENUATION_MIN = 0;
const uint8_t ATTENUATION_MAX = 15;

static bool m_diag_fem_enabled = false;
static int8_t m_diag_fem_gain = 10;
static int8_t m_diag_radio_tx_power = 0;

extern "C" {

bool vendor_diag_fem_enabled(void)
{
	return m_diag_fem_enabled;
}

int8_t vendor_diag_radio_tx_power_get(void)
{
	return m_diag_radio_tx_power;
}

int8_t vendor_diag_fem_gain_get(void)
{
	return m_diag_fem_gain;
}

void vendor_lfclk_output(const bool enable)
{
#if defined(NRF54L_SERIES)
	const nrf_grtc_clkout_t out = NRF_GRTC_CLKOUT_32K;
	const nrf_grtc_clksel_t sel = NRF_GRTC_CLKSEL_LFXO;

	if (!enable) {
		nrfy_grtc_clksel_set(NRF_GRTC, NRF_GRTC_CLKSEL_LFCLK);
		nrfy_grtc_clkout_set(NRF_GRTC, out, false);
	} else {
		nrfy_grtc_clksel_set(NRF_GRTC, sel);
		nrfy_grtc_clkout_set(NRF_GRTC, out, true);
	}
#endif
}

void vendor_hfclk_output(const bool enable, const uint8_t divisor)
{
#if defined(NRF54L_SERIES)
	const nrf_grtc_clkout_t out = NRF_GRTC_CLKOUT_FAST;

	if (!enable) {
		nrfy_grtc_clkout_divider_set(NRF_GRTC, GRTC_CLKCFG_CLKFASTDIV_Min);
		nrfy_grtc_clkout_set(NRF_GRTC, out, false);
	} else {
		nrfy_grtc_clkout_divider_set(NRF_GRTC, divisor);
		nrfy_grtc_clkout_set(NRF_GRTC, out, true);
	}
#endif
}

} // extern "C"

static otError VendorPowerLimitTable(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	otError error = OT_ERROR_NONE;
	char version[VENDOR_RADIO_POWER_LIMIT_MAX_SIZE];
	uint8_t powerLimitArraySize;
	power_limit_table_data_t powerLimitArray;

	SuccessOrExit(error = vendor_radio_power_limit_info_get(version, &powerLimitArraySize));

	otCliOutputFormat("Version=%s\r\n", (char *)version);
	otCliOutputFormat("PowerLimitArraysSize=%u\r\n", powerLimitArraySize);

	for (uint16_t region = 0; region < powerLimitArraySize; region++) {
		memset(&powerLimitArray, 0x00, sizeof(powerLimitArray));

		SuccessOrExit(
			error = vendor_radio_power_limit_table_get(
				&powerLimitArray, region, vendor_radio_power_map_tx_path_id_get()));

		otCliOutputFormat("Region=%d\r\n[%d", region, powerLimitArray[0]);
		for (int channel = 1; channel < VENDOR_RADIO_SETTINGS_RADIO_CHANNEL_COUNT;
		     channel++) {
			otCliOutputFormat(", %d", powerLimitArray[channel]);
		}
		otCliOutputFormat("]\r\n");
	}

exit:
	return error;
}

static otError VendorPowerLimitTableVersion(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	otError error = OT_ERROR_NONE;
	char version[VENDOR_RADIO_POWER_LIMIT_MAX_SIZE];
	uint8_t size;

	SuccessOrExit(error = vendor_radio_power_limit_info_get(version, &size));
	otCliOutputFormat("Version=%s\r\n", version);

exit:
	return error;
}

static otError VendorPowerLimitTableLimit(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	otError error = OT_ERROR_NONE;
	int8_t powerLimit;
	uint8_t channel;

	VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);

	channel = (uint8_t)atoi(aArgs[0]);

	SuccessOrExit(error = vendor_radio_power_limit_get(
			      channel, vendor_radio_power_map_tx_path_id_get(), &powerLimit));
	otCliOutputFormat("PowerLimit=%u\r\n", powerLimit);

exit:
	return error;
}

static otError VendorPowerLimitActiveId(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	otError error = OT_ERROR_NONE;
	uint8_t id;

	if (aArgsLength == 0) {
		SuccessOrExit(error = vendor_radio_power_limit_id_get(&id));
		otCliOutputFormat("PowerLimitTableActiveId=%u\r\n", id);
	} else if (aArgsLength == 1) {
		id = (uint8_t)atoi(aArgs[0]);
		SuccessOrExit(error = vendor_radio_power_limit_id_set(id));
	} else {
		ExitNow(error = OT_ERROR_INVALID_ARGS);
	}

exit:
	return error;
}

static otError VendorPowerLimitActiveIdInvalidate(void *aContext, uint8_t aArgsLength,
						  char *aArgs[])
{
	return vendor_radio_power_limit_id_invalidate();
}

static void PrintMapTableForChannel(const psemi_power_map_table_t *aPowerMap, uint8_t aChannel)
{
	const psemi_power_map_channel_entry_t *entry =
		&aPowerMap->channels[aChannel - MIN_802154_CHANNEL];

	otCliOutputFormat("Channel %u power:\r\n", aChannel);
	otCliOutputFormat("Output | Chip | Gain\r\n");

	for (uint8_t i = 0; i < OT_ARRAY_LENGTH(entry->entries); i++) {
		otCliOutputFormat(" %4d  | %3d  | %3d\r\n", entry->entries[i].actual_power,
				  entry->entries[i].radio_tx_power_db,
				  vendor_radio_power_map_att_to_gain(entry->entries[i].att_state));
	}
}

static otError VendorPowerMappingTable(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	otError error = OT_ERROR_NONE;
	uint16_t version;
	uint8_t numberOfPowerSettings;
	power_map_psemi_data_t *power_map_context = vendor_radio_power_map_psemi_data_get();
	const psemi_power_map_table_t *power_map_current =
		power_map_context->m_power_map.power_map_current;

	SuccessOrExit(error = vendor_radio_power_map_info_get(&version, &numberOfPowerSettings));

	otCliOutputFormat("Power Mapping Table Version: %u\r\n", version);
	otCliOutputFormat("Tx Path: %u\r\n", power_map_context->m_tx_path_id);

	if (aArgsLength == 1) {
		uint8_t channel = (uint8_t)atoi(aArgs[0]);
		VerifyOrExit(channel >= MIN_802154_CHANNEL && channel <= MAX_802154_CHANNEL,
			     error = OT_ERROR_INVALID_ARGS);
		PrintMapTableForChannel(power_map_current, channel);
	} else {
		for (uint8_t i = MIN_802154_CHANNEL; i <= MAX_802154_CHANNEL; i++) {
			PrintMapTableForChannel(power_map_current, i);
		}
	}

exit:
	return error;
}

static otError VendorPowerMappingTableVersion(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	otError error = OT_ERROR_NONE;
	uint16_t version;
	uint8_t numberOfPowerSettings;

	SuccessOrExit(error = vendor_radio_power_map_info_get(&version, &numberOfPowerSettings));

	otCliOutputFormat("mVersion=%u\r\n", version);

exit:
	return error;
}

static otError VendorPowerMappingTableTest(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	otError error = OT_ERROR_NONE;

	static uint8_t m_channel = 0;
	static int8_t m_requestedPower = 0;

	if (aArgsLength == 0) {
		int8_t femGain;
		int8_t chipPower;
		int8_t outputPower;
		uint8_t attenuation;

		VerifyOrExit(m_channel != 0, error = OT_ERROR_INVALID_STATE);
		SuccessOrExit(error = vendor_radio_power_map_test(m_channel, INT8_MIN, &chipPower,
								  &attenuation, &outputPower));
		femGain = vendor_radio_power_map_att_to_gain(attenuation);
		otCliOutputFormat("RadioTxPower=%d\r\n", chipPower);
		otCliOutputFormat("FemGain=%d\r\n", femGain);
		otCliOutputFormat("OutputPower=%d\r\n", outputPower);
	} else if (aArgsLength == 2) {
		/** @brief Configure channel and requested power for power map test. */
		m_channel = (uint8_t)atoi(aArgs[0]);
		m_requestedPower = (int8_t)atoi(aArgs[1]);

		SuccessOrExit(
			error = vendor_radio_power_map_test(0, m_requestedPower, NULL, NULL, NULL));
	} else {
		ExitNow(error = OT_ERROR_INVALID_ARGS);
	}

exit:
	return error;
}

static otError VendorPowerMappingTableClear(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	return vendor_radio_power_map_clear_flash_data();
}

static otError VendorPowerMappingTableFlashDataGet(void *aContext, uint8_t aArgsLength,
						   char *aArgs[])
{
	const uint8_t MAX_ROW_BYTES = 32U;
	uint8_t buff[MAX_ROW_BYTES];
	otError error = OT_ERROR_NONE;
	uint16_t dataSize;
	uint16_t num = 0;
	power_map_status_t status = vendor_radio_power_map_flash_data_is_valid();

	VerifyOrExit(status != POWER_MAP_STATUS_INVALID_VERSION, error = OT_ERROR_INVALID_STATE);
	VerifyOrExit(status == POWER_MAP_STATUS_OK, error = OT_ERROR_NOT_FOUND);

	SuccessOrExit(
		error = vendor_radio_power_map_read_flash_data(&dataSize, sizeof(dataSize), 0));
	dataSize = swap_uint16(dataSize) + 2;

	for (int i = 0; i < dataSize; i += num) {
		num = (dataSize - i > MAX_ROW_BYTES) ? MAX_ROW_BYTES : (dataSize - i);

		SuccessOrExit(error = vendor_radio_power_map_read_flash_data(buff, num, i));
		otCliOutputBytes(buff, num);
		otCliOutputFormat("\r\n");
	}

exit:
	return error;
}

static otError CheckDataIntegrity(uint8_t *aMap, uint16_t aLen)
{
	otError error = OT_ERROR_NONE;
	uint16_t mapSize = 0;
	uint32_t crcCalc = 0;
	uint32_t packCrc = 0;

	mapSize = (((uint16_t)aMap[0]) << 8);
	crcCalc = crc32_compute(&aMap[0], 1, NULL);
	crcCalc = crc32_compute(&aMap[1], 1, &crcCalc);
	mapSize += aMap[1];
	VerifyOrExit(mapSize > 4, error = OT_ERROR_NOT_IMPLEMENTED);
	if (mapSize + 2 == aLen) {
		for (int i = 2; i < aLen; i++) {
			if (i < aLen - 4) {
				crcCalc = crc32_compute(&aMap[i], 1, &crcCalc);
			} else {
				packCrc >>= 8;
				packCrc += aMap[i] << 24;
			}
		}

		VerifyOrExit(packCrc == crcCalc, error = OT_ERROR_PARSE);
	}

exit:
	return error;
}

static otError VendorPowerMappingTableFlashDataSet(void *aContext, uint8_t aArgsLength,
						   char *aArgs[])
{
	otError error = OT_ERROR_NONE;
	uint8_t *map = reinterpret_cast<uint8_t *>(aArgs[0]);
	uint16_t len = aArgs[0] != NULL ? strlen(aArgs[0]) : 0;

	VerifyOrExit(aArgs[0] != NULL, error = OT_ERROR_INVALID_ARGS);

	SuccessOrExit(error = ot::Utils::CmdLineParser::ParseAsHexString(aArgs[0], len, map));
	VerifyOrExit(len <= vendor_radio_power_map_flash_data_max_size_get(),
		     error = OT_ERROR_NO_BUFS);
	SuccessOrExit(error = CheckDataIntegrity(map, len));
	SuccessOrExit(error = vendor_radio_power_map_save_flash_data(map, len, 0));

	/** @brief Full power map reinit required. */
	vendor_radio_power_map_init();

	VerifyOrExit(vendor_radio_power_map_flash_data_last_flash_validation_status() ==
			     POWER_MAP_STATUS_OK,
		     error = OT_ERROR_FAILED);
exit:
	return error;
}

static otError VendorPowerMappingTableFlashData(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	otError error = OT_ERROR_NONE;

	if (aArgsLength == 0) {
		error = VendorPowerMappingTableFlashDataGet(aContext, aArgsLength, aArgs);
	} else if (aArgsLength == 1) {
		error = VendorPowerMappingTableFlashDataSet(aContext, aArgsLength, aArgs);
	} else {
		error = OT_ERROR_INVALID_ARGS;
	}

	return error;
}

static otError VendorPowerMappingTableFlashDataValid(void *aContext, uint8_t aArgsLength,
						     char *aArgs[])
{
	bool isValid =
		vendor_radio_power_map_flash_data_apply() == POWER_MAP_STATUS_OK ? true : false;

	otCliOutputFormat("Valid=%u\r\n", isValid);

	return OT_ERROR_NONE;
}

static otError VendorDiagFemEnable(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	if (aArgsLength == 0) {
		otCliOutputFormat("DiagFemEnable=%u\r\n", m_diag_fem_enabled);
	} else if (aArgsLength == 1) {
		m_diag_fem_enabled = (bool)atoi(aArgs[0]);
	} else {
		return OT_ERROR_INVALID_ARGS;
	}

	return OT_ERROR_NONE;
}

bool IsTxPowerValid(int8_t txPower)
{
	const int8_t allowedValues[] = {
#if defined(RADIO_TXPOWER_TXPOWER_Pos10dBm)
		10,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos9dBm)
		9,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos8dBm)
		8,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos7dBm)
		7,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos6dBm)
		6,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos5dBm)
		5,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos4dBm)
		4,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos3dBm)
		3,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos2dBm)
		2,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos1dBm)
		1,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_0dBm)
		0,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg1dBm)
		-1,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg2dBm)
		-2,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg3dBm)
		-3,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg4dBm)
		-4,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg5dBm)
		-5,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg6dBm)
		-6,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg7dBm)
		-7,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg8dBm)
		-8,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg9dBm)
		-9,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg10dBm)
		-10,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg12dBm)
		-12,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg14dBm)
		-14,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg16dBm)
		-16,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg20dBm)
		-20,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg26dBm)
		-26,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg30dBm)
		-30,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg40dBm)
		-40,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg46dBm)
		-46,
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Neg70dBm)
		-70,
#endif
	};

	size_t i;

	for (i = 0; i < OT_ARRAY_LENGTH(allowedValues); i++) {
		if (txPower == static_cast<int8_t>(allowedValues[i])) {
			break;
		}
	}

	return i < OT_ARRAY_LENGTH(allowedValues);
}

static otError VendorDiagRadioTxPower(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	otError error = OT_ERROR_NONE;
	int32_t txPower;

	VerifyOrExit(vendor_diag_fem_enabled(), error = OT_ERROR_INVALID_STATE);

	if (aArgsLength == 0) {
		otCliOutputFormat("%d\r\n", vendor_diag_radio_tx_power_get());
	} else if (aArgsLength == 1) {
		txPower = atoi(aArgs[0]);
		VerifyOrExit(txPower >= INT8_MIN && txPower <= INT8_MAX,
			     error = OT_ERROR_INVALID_ARGS);
		VerifyOrExit(IsTxPowerValid(txPower), error = OT_ERROR_INVALID_ARGS);
		m_diag_radio_tx_power = txPower;
	} else {
		ExitNow(error = OT_ERROR_INVALID_ARGS);
	}
exit:
	return error;
}

static int8_t FemGainToAttenuation(int8_t fem_gain)
{
	return FEM_MAX_GAIN - fem_gain;
}

static int8_t AttenuationToFemGain(int8_t attenuation)
{
	return FEM_MAX_GAIN - attenuation;
}

static otError VendorDiagAttenuation(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	otError error = OT_ERROR_NONE;
	int32_t attenuation;

	VerifyOrExit(vendor_diag_fem_enabled(), error = OT_ERROR_INVALID_STATE);

	if (aArgsLength == 0) {
		attenuation = FemGainToAttenuation(vendor_diag_fem_gain_get());
		otCliOutputFormat("%d\r\n", attenuation);
	} else if (aArgsLength == 1) {
		attenuation = atoi(aArgs[0]);
		VerifyOrExit(attenuation >= INT8_MIN && attenuation <= INT8_MAX,
			     error = OT_ERROR_INVALID_ARGS);
		VerifyOrExit(attenuation >= ATTENUATION_MIN && attenuation <= ATTENUATION_MAX,
			     error = OT_ERROR_INVALID_ARGS);
		m_diag_fem_gain = AttenuationToFemGain(attenuation);
	} else {
		ExitNow(error = OT_ERROR_INVALID_ARGS);
	}
exit:
	return error;
}

static otError VendorFem(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	otError error = OT_ERROR_NONE;
	const char *femStateStr[] = {"Disabled", "VC0", "VC1", "Bypass", "Auto"};
	mpsl_fem_psemi_state_t state;

	if (aArgsLength == 0) {
		state = mpsl_fem_psemi_state_get();
		otCliOutputFormat("%s\r\n", femStateStr[state]);
	} else if (aArgsLength == 1) {
		if (strcmp(aArgs[0], "disable") == 0) {
			state = FEM_PSEMI_STATE_DISABLED;
		} else if (strcmp(aArgs[0], "vc0") == 0) {
			state = FEM_PSEMI_STATE_LNA_ACTIVE;
		} else if (strcmp(aArgs[0], "vc1") == 0) {
			state = FEM_PSEMI_STATE_PA_ACTIVE;
		} else if (strcmp(aArgs[0], "bypass") == 0) {
			state = FEM_PSEMI_STATE_BYPASS;
		} else if (strcmp(aArgs[0], "auto") == 0) {
			state = FEM_PSEMI_STATE_AUTO;
		} else {
			ExitNow(error = OT_ERROR_INVALID_ARGS);
		}
		VerifyOrExit(!mpsl_fem_psemi_state_set(state), error = OT_ERROR_FAILED);
	} else {
		ExitNow(error = OT_ERROR_INVALID_ARGS);
	}
exit:
	return error;
}

static otError VendorFemBypass(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	otError error = OT_ERROR_NONE;
	mpsl_fem_psemi_state_t state;

	if (aArgsLength == 0) {
		state = mpsl_fem_psemi_state_get();
		otCliOutputFormat("Fem bypass is %s.\r\n",
				  state == FEM_PSEMI_STATE_BYPASS ? "on" : "off");
	} else if (aArgsLength == 1) {
		if (strcmp(aArgs[0], "on") == 0) {
			state = FEM_PSEMI_STATE_BYPASS;
		} else if (strcmp(aArgs[0], "off") == 0) {
			state = FEM_PSEMI_STATE_AUTO;
		} else {
			ExitNow(error = OT_ERROR_INVALID_ARGS);
		}
		VerifyOrExit(!mpsl_fem_psemi_state_set(state), error = OT_ERROR_FAILED);
	} else {
		ExitNow(error = OT_ERROR_INVALID_ARGS);
	}
exit:
	return error;
}

static otError VendorTemp(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	static bool m_diag_temp_initialized = false;
	otError error = OT_ERROR_NONE;
	int32_t celsius_temperature = 0;
	nrfx_temp_config_t config = NRFX_TEMP_DEFAULT_CONFIG;

	VerifyOrExit(aArgsLength == 0, error = OT_ERROR_INVALID_ARGS);

	if (!m_diag_temp_initialized) {
		VerifyOrExit(nrfx_temp_init(&config, NULL) == 0, error = OT_ERROR_FAILED);
		m_diag_temp_initialized = true;
	}
	VerifyOrExit(nrfx_temp_measure() == 0, error = OT_ERROR_FAILED);
	celsius_temperature = nrfx_temp_calculate(nrfx_temp_result_get());
	otCliOutputFormat("Measured temperature: %d\r\n", celsius_temperature);

exit:
	return error;
}

#ifdef CONFIG_THREAD_MONITOR
void process_thread_info(const struct k_thread *thread, void *user_data)
{
	thread_processing_data *data = (thread_processing_data*)user_data;
	if (data->count < MAX_THREADS) {
		data->threads[data->count].prio = thread->base.prio;

#if defined(CONFIG_THREAD_NAME)
		strncpy(data->threads[data->count].name, thread->name, CONFIG_THREAD_MAX_NAME_LEN);
		data->threads[data->count].name[CONFIG_THREAD_MAX_NAME_LEN-1] = '\0';
#endif

#if defined(CONFIG_THREAD_STACK_INFO)
		data->threads[data->count].stack =  thread->stack_info.start;
		data->threads[data->count].stack_size =  thread->stack_info.size;
#endif

		data->threads[data->count].entry =  (uintptr_t)thread->entry.pEntry;
	}
	data->count++;
}

static void print_thread_info_header(void)
{
	otCliOutputFormat("%3s ", "id");
	otCliOutputFormat("%30s ", "name");
	otCliOutputFormat("prio ");
	otCliOutputFormat("%s %12s   ", "stack", "size");
	otCliOutputFormat("entry");
	otCliOutputFormat("\r\n");
}

static void print_thread_info(uint8_t id, struct thread_info* info)
{
	otCliOutputFormat("%3u ", id);
	if (info) {
		otCliOutputFormat("%30s ", info->name);
		otCliOutputFormat("%3d  ", info->prio);
		otCliOutputFormat("%08p  %6u   ", info->stack, info->stack_size);
		otCliOutputFormat("%08p", info->entry);
	} else {
		otCliOutputFormat("Thread data not available");
	}
	otCliOutputFormat("\r\n");
}
#endif /* CONFIG_THREAD_MONITOR */

static otError VendorThreadsInfo(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	otError error = OT_ERROR_NONE;

#ifdef CONFIG_THREAD_MONITOR
	thread_processing_data data;


	VerifyOrExit(aArgsLength == 0, error = OT_ERROR_INVALID_ARGS);

	memset(&data, 0, sizeof(data));

	k_thread_foreach(process_thread_info, &data);
	print_thread_info_header();
	for (uint8_t i = 0 ; i< data.count; i++) {
		if (i < MAX_THREADS) {
			print_thread_info(i, &data.threads[i]);
		} else {
			print_thread_info(i, NULL);
		}
	}
exit:
#else /* CONFIG_THREAD_MONITOR */
	otCliOutputFormat("CONFIG_THREAD_MONITOR is disabled.\r\n");
	error = OT_ERROR_NOT_CAPABLE;
#endif /* CONFIG_THREAD_MONITOR */

	return error;
}

static otError VendorClkoutLfclk(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	otError error = OT_ERROR_NONE;

	if (aArgsLength == 1) {
		if (strcmp(aArgs[0], "start") == 0) {
			vendor_lfclk_output(true);
		} else if (strcmp(aArgs[0], "stop") == 0) {
			vendor_lfclk_output(false);
		} else {
			error = OT_ERROR_INVALID_ARGS;
		}
	} else {
		error = OT_ERROR_INVALID_ARGS;
	}
	return error;
}

static otError VendorClkoutHfclk(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	otError error = OT_ERROR_NONE;

	if (aArgsLength == 1) {
		VerifyOrExit(strcmp(aArgs[0], "stop") == 0, error = OT_ERROR_INVALID_ARGS);
		vendor_hfclk_output(false, 0);
	} else if (aArgsLength == 2) {
		int32_t divisor = atoi(aArgs[1]);
		VerifyOrExit(strcmp(aArgs[0], "start") == 0, error = OT_ERROR_INVALID_ARGS);
		VerifyOrExit(divisor >= 1 && divisor <= UINT8_MAX, error = OT_ERROR_INVALID_ARGS);
		vendor_hfclk_output(true, divisor);
	} else {
		error = OT_ERROR_INVALID_ARGS;
	}

exit:
	return error;
}

static otError VendorCurrentPowerInfo(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	otError error = OT_ERROR_NONE;
	otInstance *instance;
	uint8_t channel;
	int8_t requested_power;
	int8_t limited_power;
	int8_t soc_power;
	uint8_t att;
	int8_t fem_gain;
	int8_t real_power;

	instance = otInstanceInitSingle();
	channel = otLinkGetChannel(instance);
	SuccessOrExit(error = otPlatRadioGetTransmitPower(instance, &requested_power));

	SuccessOrExit(error = vendor_radio_power_limit_get(
			      channel, vendor_radio_power_map_tx_path_id_get(), &limited_power));
	limited_power = MIN(limited_power, requested_power);

	SuccessOrExit(error = vendor_radio_power_map_internal_tx_power_get(
			      channel, limited_power, &soc_power, &att, &real_power));
	fem_gain = vendor_radio_power_map_att_to_gain(att);

	otCliOutputFormat("Channel=%u\r\n", channel);
	otCliOutputFormat("ReqOutputPwr=%d\r\n", requested_power);
	otCliOutputFormat("LimitOutputPwr=%d\r\n", limited_power);
	otCliOutputFormat("PwrAmpValue=%d\r\n", soc_power);
	if (fem_gain > INT8_MIN) {
		otCliOutputFormat("FemGain=%d\r\n", fem_gain);
	}
	otCliOutputFormat("RealOutputPower=%d\r\n", real_power);

exit:
	return error;
}

static otError memory_read(const struct shell *sh, mem_addr_t addr, uint8_t width)
{
	otError error = OT_ERROR_NONE;
	uint32_t value;

	switch (width) {
	case 8:
		value = sys_read8(addr);
		break;
	case 16:
		value = sys_read16(addr);
		break;
	case 32:
		value = sys_read32(addr);
		break;
	default:
		otCliOutputFormat("Incorrect data width\n");
		error = OT_ERROR_INVALID_ARGS;
		break;
	}

	if (error == OT_ERROR_NONE) {
		otCliOutputFormat("Read value 0x%x\n", value);
	}

	return error;
}

static otError VendorDevMem(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	otError error = OT_ERROR_NONE;
	uint32_t address;
	uint8_t width;

	if (aArgsLength == 0) {
		otCliOutputFormat("Usage: ot vendor:devmem <address> [<width>]\r\n");
	} else {
		address = strtoul(aArgs[0], NULL, 16);
		if (aArgsLength < 2) {
			width = 32;
		} else {
			width = strtoul(aArgs[1], NULL, 10);
		}
		otCliOutputFormat("Using data width %d\n", width);
		SuccessOrExit(error = memory_read(NULL, address, width));
	}

exit:
	return error;
}

extern otError VendorUsageCpu(void *aContext, uint8_t aArgsLength, char *aArgs[]);
extern "C" otError VendorRadioTest(void *aContext, uint8_t aArgsLength, char *aArgs[]);

static const otCliCommand sExtensionCommands[] = {
	{"vendor:clkout:lfclk", VendorClkoutLfclk},
	{"vendor:clkout:hfclk", VendorClkoutHfclk},
	{"vendor:current:power:info", VendorCurrentPowerInfo},
	{"vendor:devmem", VendorDevMem},
	{"vendor:diag:attenuation", VendorDiagAttenuation},
	{"vendor:diag:fem:enable", VendorDiagFemEnable},
	{"vendor:diag:radiotxpower", VendorDiagRadioTxPower},
	{"vendor:fem", VendorFem},
	{"vendor:fem:bypass", VendorFemBypass},
	{"vendor:power:limit:active:id", VendorPowerLimitActiveId},
	{"vendor:power:limit:active:id:invalidate", VendorPowerLimitActiveIdInvalidate},
	{"vendor:power:limit:table", VendorPowerLimitTable},
	{"vendor:power:limit:table:limit", VendorPowerLimitTableLimit},
	{"vendor:power:limit:table:version", VendorPowerLimitTableVersion},
	{"vendor:power:mapping:table", VendorPowerMappingTable},
	{"vendor:power:mapping:table:clear", VendorPowerMappingTableClear},
	{"vendor:power:mapping:table:flash:data", VendorPowerMappingTableFlashData},
	{"vendor:power:mapping:table:flash:data:valid", VendorPowerMappingTableFlashDataValid},
	{"vendor:power:mapping:table:test", VendorPowerMappingTableTest},
	{"vendor:power:mapping:table:version", VendorPowerMappingTableVersion},
	{"vendor:temp", VendorTemp},
	{"vendor:threads:info", VendorThreadsInfo},
#ifdef CONFIG_OPENTHREAD_CLI_VENDOR_CPU_USAGE
	{"vendor:usage:cpu", VendorUsageCpu},
#endif
#ifdef CONFIG_OPENTHREAD_CLI_VENDOR_RADIO_TEST
	{"vendor:radio_test", VendorRadioTest},
#endif
};

void otCliVendorSetUserCommands(void)
{
	IgnoreError(otCliSetUserCommands(sExtensionCommands, OT_ARRAY_LENGTH(sExtensionCommands),
					 NULL));
}
