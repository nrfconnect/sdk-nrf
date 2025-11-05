/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc/rpc_utils/die_temp.h>
#include <rpc_utils_group.h>

#include <nrf_rpc_cbor.h>
#include <zcbor_common.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/toolchain.h>

static const struct device *temp_dev = DEVICE_DT_GET(DT_NODELABEL(temp));

struct die_temp_response {
	uint8_t error;
	int32_t temperature_centi;
};

/**
 * @brief Read die temperature from sensor.
 *
 * This function performs the complete sequence to read the die temperature:
 * checks device availability, fetches sensor sample, and retrieves the
 * temperature value from the sensor channel.
 *
 * @param[out] sensor_val Pointer to store the sensor value. Must not be NULL.
 *
 * @return Error code indicating the result of the operation:
 * @retval NRF_RPC_DIE_TEMP_SUCCESS               Temperature successfully read
 * @retval NRF_RPC_DIE_TEMP_ERR_NOT_READY         Sensor device not ready
 * @retval NRF_RPC_DIE_TEMP_ERR_FETCH_FAILED      Failed to fetch sensor sample
 * @retval NRF_RPC_DIE_TEMP_ERR_CHANNEL_GET_FAILED Failed to get sensor channel value
 */
static enum nrf_rpc_die_temp_error read_die_temp_sensor(struct sensor_value *sensor_val)
	__attribute_nonnull(1);

static enum nrf_rpc_die_temp_error read_die_temp_sensor(struct sensor_value *sensor_val)
{
	int ret;

	if (!device_is_ready(temp_dev)) {
		return NRF_RPC_DIE_TEMP_ERR_NOT_READY;
	}

	ret = sensor_sample_fetch_chan(temp_dev, SENSOR_CHAN_DIE_TEMP);
	if (ret != 0) {
		return NRF_RPC_DIE_TEMP_ERR_FETCH_FAILED;
	}

	ret = sensor_channel_get(temp_dev, SENSOR_CHAN_DIE_TEMP, sensor_val);
	if (ret != 0) {
		return NRF_RPC_DIE_TEMP_ERR_CHANNEL_GET_FAILED;
	}

	return NRF_RPC_DIE_TEMP_SUCCESS;
}

static void die_temp_get_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				 void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	struct die_temp_response response = {
		.error = NRF_RPC_DIE_TEMP_SUCCESS,
		.temperature_centi = 0,
	};
	struct sensor_value sensor_val;

	nrf_rpc_cbor_decoding_done(group, ctx);

	response.error = read_die_temp_sensor(&sensor_val);
	if (response.error == NRF_RPC_DIE_TEMP_SUCCESS) {
		response.temperature_centi = (int32_t)sensor_value_to_centi(&sensor_val);
	}

	size_t cbor_size = 0;

	/* Error code size */
	cbor_size += zcbor_header_len(response.error);

	/* Temperature size (depends on actual value) */
	if (response.temperature_centi >= 0) {
		cbor_size += zcbor_header_len(response.temperature_centi);
	} else {
		cbor_size += zcbor_header_len(-response.temperature_centi - 1);
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, cbor_size);
	nrf_rpc_encode_uint(&rsp_ctx, response.error);
	nrf_rpc_encode_int(&rsp_ctx, response.temperature_centi);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group, die_temp_get, RPC_UTIL_DIE_TEMP_GET, die_temp_get_handler,
			 NULL);
