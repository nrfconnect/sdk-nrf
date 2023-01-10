/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* @file
 * @brief NRF Wi-Fi radio FICR programming functions
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include "rpu_if.h"
#include "rpu_hw_if.h"
#include "ficr_prog.h"


LOG_MODULE_DECLARE(otp_prog, CONFIG_WIFI_LOG_LEVEL);

static void write_word(unsigned int addr, unsigned int data)
{
	rpu_write(addr, &data, 4);
}

static void read_word(unsigned int addr, unsigned int *data)
{
	rpu_read(addr, data, 4);
}

unsigned int check_protection(unsigned int *buff, unsigned int off1, unsigned int off2,
		    unsigned int off3, unsigned int off4)
{
	if ((buff[off1] == OTP_PROGRAMMED) &&
		(buff[off2] == OTP_PROGRAMMED)  &&
		(buff[off3] == OTP_PROGRAMMED) &&
		(buff[off4] == OTP_PROGRAMMED))
		return OTP_PROGRAMMED;
	else if ((buff[off1] == OTP_FRESH_FROM_FAB) &&
		(buff[off2] == OTP_FRESH_FROM_FAB) &&
		(buff[off3] == OTP_FRESH_FROM_FAB) &&
		(buff[off4] == OTP_FRESH_FROM_FAB))
		return OTP_FRESH_FROM_FAB;
	else if ((buff[off1] == OTP_ENABLE_PATTERN) &&
		(buff[off2] == OTP_ENABLE_PATTERN) &&
		(buff[off3] == OTP_ENABLE_PATTERN) &&
		(buff[off4] == OTP_ENABLE_PATTERN))
		return OTP_ENABLE_PATTERN;
	else
		return OTP_INVALID;
}


static void set_otp_timing_reg_40mhz(void)
{
	write_word(OTP_TIMING_REG1_ADDR, OTP_TIMING_REG1_VAL);
	write_word(OTP_TIMING_REG2_ADDR, OTP_TIMING_REG2_VAL);
}

static int poll_otp_ready(void)
{
	int otp_mem_status = 0;
	int poll = 0;

	while (poll != 100) {
		read_word(OTP_POLL_ADDR, &otp_mem_status);

		if ((otp_mem_status & OTP_READY) == OTP_READY) {
			return 0;
		}
		poll++;
	}
	LOG_ERR("OTP is not ready\n");
	return -ENOEXEC;
}


static int req_otp_standby_mode(void)
{
	write_word(OTP_RWSBMODE_ADDR, 0x0);
	return poll_otp_ready();
}


static int otp_wr_voltage_2V5(void)
{
	int err;

	err = req_otp_standby_mode();

	if (err) {
		LOG_ERR("Failed Setting OTP voltage IOVDD to 2.5V\n");
		return -ENOEXEC;
	}
	write_word(OTP_VOLTCTRL_ADDR, OTP_VOLTCTRL_2V5);
	return 0;
}

static int poll_otp_read_valid(void)
{
	int otp_mem_status = 0;
	int poll = 0;

	while (poll < 100) {
		read_word(OTP_POLL_ADDR, &otp_mem_status);

		if ((otp_mem_status & OTP_READ_VALID) == OTP_READ_VALID) {
			return 0;
		}
		poll++;
	}
	LOG_ERR("failed poll_otp_read_valid()\n");
	return -ENOEXEC;
}

static int poll_otp_wrdone(void)
{
	int otp_mem_status = 0;
	int poll = 0;

	while (poll < 100) {
		read_word(OTP_POLL_ADDR, &otp_mem_status);

		if ((otp_mem_status & OTP_WR_DONE) == OTP_WR_DONE) {
			return 0;
		}
		poll++;
	}
	LOG_ERR("failed poll_otp_wrdone()\n");
	return -ENOEXEC;
}

static int req_otp_read_mode(void)
{
	write_word(OTP_RWSBMODE_ADDR, OTP_READ_MODE);
	return poll_otp_ready();
}


static int req_otp_byte_write_mode(void)
{
	write_word(OTP_RWSBMODE_ADDR, OTP_BYTE_WRITE_MODE);
	return poll_otp_ready();
}

static unsigned int read_otp_location(unsigned int offset, unsigned int *read_val)
{
	int err;

	write_word(OTP_RDENABLE_ADDR,  offset);
	err = poll_otp_read_valid();
	if (err) {
		LOG_ERR("OTP read failed\n");
		return err;
	}
	read_word(OTP_READREG_ADDR, read_val);

	return 0;
}

static int  write_otp_location(unsigned int otp_location_offset, unsigned int otp_data)
{
	write_word(OTP_WRENABLE_ADDR,  otp_location_offset);
	write_word(OTP_WRITEREG_ADDR,  otp_data);

	return poll_otp_wrdone();
}


static int otp_rd_voltage_1V8(void)
{
	int err;

	err = req_otp_standby_mode();
	if (err) {
		LOG_ERR("error in %s\n", __func__);
		return err;
	}
	write_word(OTP_VOLTCTRL_ADDR, OTP_VOLTCTRL_1V8);

	return 0;
}


int write_otp_memory(unsigned int otp_addr, unsigned int *write_val)
{
	int err;
	int	mask_val;

	err = poll_otp_ready();
	if (err) {
		LOG_ERR("err in otp ready poll\n");
		return err;
	}

	set_otp_timing_reg_40mhz();

	err = otp_wr_voltage_2V5();
	if (err) {
		LOG_ERR("error in write_voltage 2V5\n");
		goto _exit_otp_write;
	}

	err = req_otp_byte_write_mode();
	if (err) {
		LOG_ERR("error in OTP byte write mode\n");
		goto _exit_otp_write;
	}

	switch (otp_addr) {
	case REGION_PROTECT:
		write_otp_location(REGION_PROTECT, write_val[0]);
		write_otp_location(REGION_PROTECT+1, write_val[0]);
		write_otp_location(REGION_PROTECT+2, write_val[0]);
		write_otp_location(REGION_PROTECT+3, write_val[0]);

		LOG_INF("Written REGION_PROTECT0 (0x%x) : 0x%04x\n",
						(REGION_PROTECT << 2), write_val[0]);
		LOG_INF("Written REGION_PROTECT1 (0x%x) : 0x%04x\n",
						(REGION_PROTECT+1) << 2, write_val[0]);
		LOG_INF("Written REGION_PROTECT2 (0x%x) : 0x%04x\n",
						(REGION_PROTECT+2) << 2, write_val[0]);
		LOG_INF("Written REGION_PROTECT3 (0x%x) : 0x%04x\n",
						(REGION_PROTECT+3) << 2, write_val[0]);
		break;
	case QSPI_KEY:
		mask_val = QSPI_KEY_FLAG_MASK;
		write_otp_location(QSPI_KEY, write_val[0]);
		write_otp_location(QSPI_KEY+1, write_val[1]);
		write_otp_location(QSPI_KEY+2, write_val[2]);
		write_otp_location(QSPI_KEY+3, write_val[3]);
		write_otp_location(REGION_DEFAULTS, mask_val);

		LOG_INF("Written QSPI_KEY0 (0x%x) : 0x%04x\n",
						(QSPI_KEY+0) << 2, write_val[0]);
		LOG_INF("Written QSPI_KEY1 (0x%x) : 0x%04x\n",
						(QSPI_KEY+1) << 2, write_val[1]);
		LOG_INF("Written QSPI_KEY2 (0x%x) : 0x%04x\n",
						(QSPI_KEY+2) << 2, write_val[2]);
		LOG_INF("Written QSPI_KEY3 (0x%x) : 0x%04x\n",
						(QSPI_KEY+3) << 2, write_val[3]);
		LOG_INF("Written REGION_DEFAULTS (0x%x) : 0x%04x\n",
						(REGION_DEFAULTS) << 2, mask_val);
		break;
	case MAC0_ADDR:
		mask_val = MAC0_ADDR_FLAG_MASK;
		write_otp_location(MAC0_ADDR, write_val[0]);
		write_otp_location(MAC0_ADDR+1, write_val[1]);
		write_otp_location(REGION_DEFAULTS, mask_val);

		LOG_INF("Written MAC address 0\n");
		LOG_INF("mac addr 0 : Reg1 (0x%x) = 0x%04x\n",
						(MAC0_ADDR) << 2, write_val[0]);
		LOG_INF("mac addr 0 : Reg2 (0x%x) = 0x%04x\n",
						(MAC0_ADDR+1) << 2, write_val[1]);
		LOG_INF("Written REGION_DEFAULTS (0x%x) : 0x%04x\n",
						(REGION_DEFAULTS) << 2, mask_val);
		break;
	case MAC1_ADDR:
		mask_val = MAC1_ADDR_FLAG_MASK;
		write_otp_location(MAC1_ADDR, write_val[0]);
		write_otp_location(MAC1_ADDR+1, write_val[1]);
		write_otp_location(REGION_DEFAULTS, mask_val);

		LOG_INF("Written MAC address 1\n");
		LOG_INF("mac addr 0 : Reg1 (0x%x) = 0x%04x\n",
						(MAC1_ADDR) << 2, write_val[0]);
		LOG_INF("mac addr 0 : Reg2 (0x%x) = 0x%04x\n",
						(MAC1_ADDR+1) << 2, write_val[1]);
		LOG_INF("Written REGION_DEFAULTS (0x%x) : 0x%04x\n",
						(REGION_DEFAULTS) << 2, mask_val);
		break;
	case CALIB_XO:
		mask_val = CALIB_XO_FLAG_MASK;
		write_otp_location(CALIB_XO, write_val[0]);
		write_otp_location(REGION_DEFAULTS, mask_val);

		LOG_INF("Written CALIB_XO (0x%x) to 0x%04x\n",
						CALIB_XO << 2, write_val[0]);
		LOG_INF("Written REGION_DEFAULTS (0x%x) : 0x%04x\n",
						(REGION_DEFAULTS) << 2, mask_val);
		break;
	case CALIB_PDADJM7:
		mask_val = CALIB_PDADJM7_FLAG_MASK;
		write_otp_location(CALIB_PDADJM7, write_val[0]);
		write_otp_location(REGION_DEFAULTS, mask_val);

		LOG_INF("Written CALIB_PDADJM7 (0x%x) to 0x%04x\n",
						CALIB_PDADJM7 << 2, write_val[0]);
		LOG_INF("Written REGION_DEFAULTS (0x%x) : 0x%04x\n",
						(REGION_DEFAULTS) << 2, mask_val);
		break;
	case CALIB_PDADJM0:
		mask_val = CALIB_PDADJM0_FLAG_MASK;
		write_otp_location(CALIB_PDADJM0, write_val[0]);
		write_otp_location(REGION_DEFAULTS, mask_val);

		LOG_INF("Written CALIB_PDADJM0 (0x%x) to 0x%04x\n",
						CALIB_PDADJM0 << 2, write_val[0]);
		LOG_INF("Written REGION_DEFAULTS (0x%x) : 0x%04x\n",
						(REGION_DEFAULTS) << 2, mask_val);
		break;
	case CALIB_PWR2G:
		mask_val = CALIB_PWR2G_FLAG_MASK;
		write_otp_location(CALIB_PWR2G, write_val[0]);
		write_otp_location(REGION_DEFAULTS, mask_val);

		LOG_INF("Written CALIB_PWR2G (0x%x) to 0x%04x\n",
						CALIB_PWR2G << 2, write_val[0]);
		LOG_INF("Written REGION_DEFAULTS (0x%x) : 0x%04x\n",
						(REGION_DEFAULTS) << 2, mask_val);
		break;
	case CALIB_PWR5GM7:
		mask_val = CALIB_PWR5GM7_FLAG_MASK;
		write_otp_location(CALIB_PWR5GM7, write_val[0]);
		write_otp_location(REGION_DEFAULTS, mask_val);

		LOG_INF("Written CALIB_PWR5GM7 (0x%x) to 0x%04x\n",
						CALIB_PWR5GM7 << 2, write_val[0]);
		LOG_INF("Written REGION_DEFAULTS (0x%x) : 0x%04x\n",
						(REGION_DEFAULTS) << 2, mask_val);
		break;
	case CALIB_PWR5GM0:
		mask_val = CALIB_PWR5GM0_FLAG_MASK;
		write_otp_location(CALIB_PWR5GM0, write_val[0]);
		write_otp_location(REGION_DEFAULTS, mask_val);

		LOG_INF("Written CALIB_PWR5GM0 (0x%x) to 0x%04x\n",
						CALIB_PWR5GM0 << 2, write_val[0]);
		LOG_INF("Written REGION_DEFAULTS (0x%x) : 0x%04x\n",
						(REGION_DEFAULTS) << 2, mask_val);
		break;
	case CALIB_RXGNOFF:
		mask_val = CALIB_RXGNOFF_FLAG_MASK;
		write_otp_location(CALIB_RXGNOFF, write_val[0]);
		write_otp_location(REGION_DEFAULTS, mask_val);

		LOG_INF("Written CALIB_RXGNOFF (0x%x) to 0x%04x\n",
						CALIB_RXGNOFF << 2, write_val[0]);
		LOG_INF("Written REGION_DEFAULTS (0x%x) : 0x%04x\n",
						(REGION_DEFAULTS) << 2, mask_val);
		break;
	case CALIB_TXPOWBACKOFFT:
		mask_val = CALIB_TXPOWBACKOFFT_FLAG_MASK;
		write_otp_location(CALIB_TXPOWBACKOFFT, write_val[0]);
		write_otp_location(REGION_DEFAULTS, mask_val);

		LOG_INF("Written CALIB_TXPOWBACKOFFT (0x%x) to 0x%04x\n",
						CALIB_TXPOWBACKOFFT << 2, write_val[0]);
		LOG_INF("Written REGION_DEFAULTS (0x%x) : 0x%04x\n",
						(REGION_DEFAULTS) << 2, mask_val);
		break;
	case CALIB_TXPOWBACKOFFV:
		mask_val = CALIB_TXPOWBACKOFFV_FLAG_MASK;
		write_otp_location(CALIB_TXPOWBACKOFFV, write_val[0]);
		write_otp_location(REGION_DEFAULTS, mask_val);

		LOG_INF("Written CALIB_TXPOWBACKOFFV (0x%x) to 0x%04x\n",
						CALIB_TXPOWBACKOFFV << 2, write_val[0]);
		LOG_INF("Written REGION_DEFAULTS (0x%x) : 0x%04x\n",
						(REGION_DEFAULTS) << 2, mask_val);
		break;
	case REGION_DEFAULTS:
		write_otp_location(REGION_DEFAULTS, write_val[0]);

		LOG_INF("Written REGION_DEFAULTS (0x%x) to 0x%04x\n",
						REGION_DEFAULTS << 2, write_val[0]);
		break;
	default:
		LOG_ERR("unknown field received: %d\n", otp_addr);

	}

_exit_otp_write:
	err  = req_otp_standby_mode();
	err |= otp_rd_voltage_1V8();
	return err;
}

int read_otp_memory(unsigned int otp_addr, unsigned int *read_val, int len)
{
	int	err;

	err = poll_otp_ready();
	if (err) {
		LOG_ERR("err in otp ready poll\n");
		return -ENOEXEC;
	}

	set_otp_timing_reg_40mhz();

	err = otp_rd_voltage_1V8();
	if (err) {
		LOG_ERR("error in read_voltage 1V8\n");
		return -ENOEXEC;
	}

	err = req_otp_read_mode();
	if (err) {
		LOG_ERR("error in req_otp_read_mode()\n");
		return -ENOEXEC;
	}

	for (int i = 0; i < len; i++) {
		read_otp_location(otp_addr + i, &read_val[i]);
	}

	return req_otp_standby_mode();
}
