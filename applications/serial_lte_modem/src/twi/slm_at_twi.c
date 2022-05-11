/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <stdio.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_twi.h"

LOG_MODULE_REGISTER(slm_twi, CONFIG_SLM_LOG_LEVEL);

#if defined(CONFIG_HAS_HW_NRF_TWIM3)
#define TWI_MAX_INSTANCE	4
#elif defined(CONFIG_HAS_HW_NRF_TWIM2)
#define TWI_MAX_INSTANCE	3
#elif defined(CONFIG_HAS_HW_NRF_TWIM1)
#define TWI_MAX_INSTANCE	2
#elif defined(CONFIG_HAS_HW_NRF_TWIM0)
#define TWI_MAX_INSTANCE	1
#endif
#define TWI_ADDR_LEN		2
#define TWI_DATA_LEN		255

#if (TWI_DATA_LEN * 2) > (CONFIG_SLM_SOCKET_RX_MAX * 2)
# error "Please specify smaller TWI_DATA_LEN"
#endif

static const struct device *slm_twi_dev[TWI_MAX_INSTANCE];
static uint8_t twi_data[TWI_DATA_LEN * 2 + 1];

/* global variable defined in different files */
extern struct at_param_list at_param_list;
extern char rsp_buf[SLM_AT_CMD_RESPONSE_MAX_LEN];

static void do_twi_list(void)
{
	memset(rsp_buf, 0, sizeof(rsp_buf));
	sprintf(rsp_buf, "\r\n#XTWILS: ");

	for (int i = 0; i < TWI_MAX_INSTANCE; i++) {
		if (slm_twi_dev[i]) {
			sprintf(rsp_buf + strlen(rsp_buf), "%d,", i);
		}
	}
	if (strlen(rsp_buf) > 0) {
		strcat(rsp_buf, "\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
	}
}

static int do_twi_write(uint16_t index, uint16_t dev_addr,
			const uint8_t *twi_data_ascii, uint16_t ascii_len)
{
	int ret = -EINVAL;

	if (!slm_twi_dev[index]) {
		LOG_ERR("TWI device is not opened");
		return ret;
	}

	/* Decode hex string to hex array */
	memset(rsp_buf, 0, sizeof(rsp_buf));
	ret = slm_util_atoh(twi_data_ascii, ascii_len, rsp_buf, ascii_len / 2);
	if (ret < 0) {
		LOG_ERR("Fail to decode hex string to hex array");
		return ret;
	}

	ret = i2c_write(slm_twi_dev[index], rsp_buf, ret, dev_addr);
	if (ret < 0) {
		LOG_ERR("Fail to write twi data at address: %hx", dev_addr);
	}

	return ret;
}

static int do_twi_read(uint16_t index, uint16_t dev_addr, uint8_t num_read)
{
	int ret = -EINVAL;

	if (!slm_twi_dev[index]) {
		LOG_ERR("TWI device is not opened");
		return ret;
	}

	if (num_read > TWI_DATA_LEN) {
		LOG_ERR("Not enough buffer. Increase TWI_DATA_LEN");
		return -ENOBUFS;
	}

	memset(twi_data, 0, sizeof(twi_data));
	ret = i2c_read(slm_twi_dev[index], twi_data, (uint32_t)num_read, dev_addr);
	if (ret < 0) {
		LOG_ERR("Fail to read twi data");
		return ret;
	}
	memset(rsp_buf, 0, sizeof(rsp_buf));
	ret = slm_util_htoa(twi_data, num_read, rsp_buf, num_read * 2);
	if (ret > 0) {
		sprintf(rsp_buf + ret, "\r\n#XTWIR: ");
		rsp_send(rsp_buf + ret, strlen(rsp_buf + ret));
		rsp_send(rsp_buf, ret);
		rsp_send("\r\n", 2);
		ret = 0;
	} else {
		LOG_ERR("hex convert error: %d", ret);
		ret = -EINVAL;
	}

	return ret;
}

static int do_twi_write_read(uint16_t index, uint16_t dev_addr,
			     const uint8_t *twi_data_ascii, uint16_t ascii_len,
			     uint16_t num_read)
{
	int ret = -EINVAL;

	if (!slm_twi_dev[index]) {
		LOG_ERR("TWI device is not opened");
		return ret;
	}

	/* Decode hex string to hex array */
	memset(rsp_buf, 0, sizeof(rsp_buf));
	ret = slm_util_atoh(twi_data_ascii, ascii_len, rsp_buf, ascii_len / 2);
	if (ret < 0) {
		LOG_ERR("Fail to decode hex string to hex array");
		return ret;
	}
	memset(twi_data, 0, sizeof(twi_data));
	ret = i2c_write_read(slm_twi_dev[index], dev_addr,
			     rsp_buf, ret,
			     twi_data, num_read);
	if (ret < 0) {
		LOG_ERR("Fail to write and read data at address: %hx", dev_addr);
		return ret;
	}
	/* Encode hex arry to hex string */
	memset(rsp_buf, 0, sizeof(rsp_buf));
	ret = slm_util_htoa(twi_data, num_read, rsp_buf, num_read * 2);
	if (ret > 0) {
		sprintf(rsp_buf + ret, "\r\n#XTWIWR: ");
		rsp_send(rsp_buf + ret, strlen(rsp_buf + ret));
		rsp_send(rsp_buf, ret);
		rsp_send("\r\n", 2);
		ret = 0;
	} else {
		LOG_ERR("hex convert error: %d", ret);
		ret = -EINVAL;
	}

	return ret;
}

/**@brief handle AT#XTWILS commands
 *  AT#XTWILS
 *  AT#XTWILS? READ command not supported
 *  AT#XTWILS=? TEST command not supported
 */
int handle_at_twi_list(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		do_twi_list();
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XTWIW commands
 *  AT#XTWIW=<index>,<dev_addr>,<data>
 *  AT#XTWIW? READ command not supported
 *  AT#XTWIW=?
 */
int handle_at_twi_write(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t index, dev_addr;
	uint8_t twi_addr_ascii[TWI_ADDR_LEN + 1];
	size_t ascii_len;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&at_param_list) != 4) {
			LOG_ERR("Wrong input parameters");
			return -EINVAL;
		}
		err = at_params_unsigned_short_get(&at_param_list, 1, &index);
		if (err < 0) {
			LOG_ERR("Fail to get twi index: %d", err);
			return err;
		}
		ascii_len = TWI_ADDR_LEN + 1;
		err = util_string_get(&at_param_list, 2, twi_addr_ascii, &ascii_len);
		if (err < 0) {
			LOG_ERR("Fail to get device address");
			return err;
		}
		sscanf(twi_addr_ascii, "%hx", &dev_addr);
		LOG_DBG("dev_addr: %hx", dev_addr);
		ascii_len = sizeof(twi_data);
		err = util_string_get(&at_param_list, 3, twi_data, &ascii_len);
		if (err) {
			return err;
		}
		LOG_DBG("Data to write: %s", log_strdup(twi_data));
		err = do_twi_write(index, dev_addr, twi_data, (uint16_t)ascii_len);
		break;
	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "#XTWIW: <index>,<dev_addr>,<data>\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;
	default:
		break;
	}

	return err;
}

/**@brief handle AT#XTWIR commands
 *  AT#XTWIR=<index>,<dev_addr>,<num_read>
 *  AT#XTWIR? READ command not supported
 *  AT#XTWIR=?
 */
int handle_at_twi_read(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t index, dev_addr, num_read;
	uint8_t twi_addr_ascii[TWI_ADDR_LEN + 1];
	size_t ascii_len;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&at_param_list, 1, &index);
		if (err < 0) {
			LOG_ERR("Fail to get twi index: %d", err);
			return err;
		}
		ascii_len = TWI_ADDR_LEN + 1;
		err = util_string_get(&at_param_list, 2, twi_addr_ascii, &ascii_len);
		if (err < 0) {
			LOG_ERR("Fail to get device address: %d", err);
			return err;
		}
		sscanf(twi_addr_ascii, "%hx", &dev_addr);
		LOG_DBG("dev_addr: %hx", dev_addr);
		err = at_params_unsigned_short_get(&at_param_list, 3, &num_read);
		if (err < 0) {
			LOG_ERR("Fail to get bytes to read: %d", err);
			return err;
		}
		if (num_read > TWI_DATA_LEN) {
			LOG_ERR("No enough buffer to read %d bytes", num_read);
			return -ENOBUFS;
		}
		err = do_twi_read(index, dev_addr, (uint8_t)num_read);
		if (err < 0) {
			return err;
		}
		break;
	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "#XTWIR: <index>,<dev_addr>,<num_read>\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;
	default:
		break;
	}

	return err;
}

/**@brief handle AT#XTWIWR commands
 *  AT#XTWIWR=<index>,<dev_addr>,<data>,<num_read>
 *  AT#XTWIWR? READ command not supported
 *  AT#XTWIWR=?
 */
int handle_at_twi_write_read(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t index, dev_addr, num_read;
	uint8_t twi_addr_ascii[TWI_ADDR_LEN + 1];
	size_t ascii_len;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&at_param_list, 1, &index);
		if (err < 0) {
			LOG_ERR("Fail to get twi index: %d", err);
			return err;
		}
		ascii_len = TWI_ADDR_LEN + 1;
		err = util_string_get(&at_param_list, 2, twi_addr_ascii, &ascii_len);
		if (err < 0) {
			LOG_ERR("Fail to get device address");
			return err;
		}
		sscanf(twi_addr_ascii, "%hx", &dev_addr);
		LOG_DBG("dev_addr: %hx", dev_addr);
		ascii_len = sizeof(twi_data);
		err = util_string_get(&at_param_list, 3, twi_data, &ascii_len);
		if (err) {
			return err;
		}
		LOG_DBG("Data to write: %s", log_strdup(twi_data));
		err = at_params_unsigned_short_get(&at_param_list, 4, &num_read);
		if (err < 0) {
			LOG_ERR("Fail to get twi index: %d", err);
			return err;
		}
		if (num_read > TWI_DATA_LEN) {
			LOG_ERR("No enough buffer to read %d bytes", num_read);
			return -ENOBUFS;
		}
		err = do_twi_write_read(index, dev_addr,
					twi_data, (uint16_t)ascii_len,
					num_read);
		if (err < 0) {
			return err;
		}
		break;
	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "#XTWIWR: <index>,<dev_addr>,<data>,<num_read>\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;
	default:
		break;
	}

	return err;
}

int slm_at_twi_init(void)
{
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(i2c0), nordic_nrf_twim, okay)
	slm_twi_dev[0] = device_get_binding(DT_LABEL(DT_NODELABEL(i2c0)));
	LOG_DBG("bind %s", slm_twi_dev[0]->name);
#endif
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(i2c1), nordic_nrf_twim, okay)
	slm_twi_dev[1] = device_get_binding(DT_LABEL(DT_NODELABEL(i2c1)));
	LOG_DBG("bind %s", slm_twi_dev[1]->name);
#endif
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(i2c2), nordic_nrf_twim, okay)
	slm_twi_dev[2] = device_get_binding(DT_LABEL(DT_NODELABEL(i2c2)));
	LOG_DBG("bind %s", slm_twi_dev[2]->name);
#endif
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(i2c3), nordic_nrf_twim, okay)
	slm_twi_dev[3] = device_get_binding(DT_LABEL(DT_NODELABEL(i2c3)));
	LOG_DBG("bind %s", slm_twi_dev[3]->name);
#endif

	return 0;
}

int slm_at_twi_uninit(void)
{
	return 0;
}
