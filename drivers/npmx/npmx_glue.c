/*
 * Copyright (c) 2022, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <npmx.h>
#include <drivers/i2c.h>
#include <zephyr/sys/byteorder.h>

int npmx_backend_i2c_write(npmx_backend_instance_t const *p_inst, uint16_t register_address,
			   uint8_t *p_data, uint32_t num_of_bytes)
{
	const struct device *i2c_dev = (struct device *)p_inst->p_backend;
	uint8_t wr_addr[2];
	struct i2c_msg msgs[2];

	/* npmx register address */
	sys_put_be16(register_address, wr_addr);

	/* Setup I2C messages */

	/* Send the address to write to */
	msgs[0].buf = wr_addr;
	msgs[0].len = 2U;
	msgs[0].flags = I2C_MSG_WRITE;

	/* Data to be written, and STOP after this. */
	msgs[1].buf = p_data;
	msgs[1].len = num_of_bytes;
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, msgs, 2, p_inst->address);
}

int npmx_backend_i2c_read(npmx_backend_instance_t const *p_inst, uint16_t register_address,
			  uint8_t *p_data, uint32_t num_of_bytes)
{
	const struct device *i2c_dev = p_inst->p_backend;
	uint8_t wr_addr[2];
	struct i2c_msg msgs[2];

	/* npmx register address */
	sys_put_be16(register_address, wr_addr);

	/* Setup I2C messages */

	/* Send the address to read from */
	msgs[0].buf = wr_addr;
	msgs[0].len = 2U;
	msgs[0].flags = I2C_MSG_WRITE;

	/* Read from device. STOP after this. */
	msgs[1].buf = p_data;
	msgs[1].len = num_of_bytes;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, msgs, 2, p_inst->address);
}
