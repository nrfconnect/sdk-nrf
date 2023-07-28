/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* @file
 * @brief NRF Wi-Fi radio test shell module
 */

#include <zephyr/logging/log.h>
#include <zephyr_fmac_main.h>
#include <util.h>
#include <nrf_wifi_radio_test_shell.h>
#include "ficr_prog.h"

LOG_MODULE_REGISTER(otp_prog, CONFIG_WIFI_LOG_LEVEL);

static void disp_location_status(const struct shell *shell, char *region, unsigned int ret)
{
	switch (ret) {

	case OTP_PROGRAMMED:
		shell_fprintf(shell, SHELL_INFO, "%s Region is locked!!!\n", region);
		break;
	case OTP_ENABLE_PATTERN:
		shell_fprintf(shell, SHELL_INFO, "%s Region is open for R/W\n", region);
		break;
	case OTP_FRESH_FROM_FAB:
		shell_fprintf(shell, SHELL_INFO,
				"%s Region is unprogrammed - program to enable R/W\n", region);
		break;
	default:
		shell_fprintf(shell, SHELL_ERROR, "%s Region is in invalid state\n", region);
		break;
	}
	shell_fprintf(shell, SHELL_INFO, "\n");
}

static void disp_fields_status(const struct shell *shell, unsigned int flags)
{
	if (flags & (~QSPI_KEY_FLAG_MASK)) {
		shell_fprintf(shell, SHELL_INFO, "QSPI Keys are not programmed in OTP\n");
	} else {
		shell_fprintf(shell, SHELL_INFO, "QSPI Keys are programmed in OTP\n");
	}

	if (flags & (~MAC0_ADDR_FLAG_MASK)) {
		shell_fprintf(shell, SHELL_INFO, "MAC0 Address are not programmed in OTP\n");
	} else {
		shell_fprintf(shell, SHELL_INFO, "MAC0 Address is programmed in OTP\n");
	}

	if (flags & (~MAC1_ADDR_FLAG_MASK)) {
		shell_fprintf(shell, SHELL_INFO, "MAC1 Address are not programmed in OTP\n");
	} else {
		shell_fprintf(shell, SHELL_INFO, "MAC1 Address is programmed in OTP\n");
	}

	if (flags & (~CALIB_XO_FLAG_MASK)) {
		shell_fprintf(shell, SHELL_INFO, "CALIB_XO is not programmed in OTP\n");
	} else {
		shell_fprintf(shell, SHELL_INFO, "CALIB_XO is programmed in OTP\n");
	}
}


static int nrf_wifi_radio_test_otp_get_status(const struct shell *shell,
					size_t argc,
					char  *argv[])
{
	unsigned int ret, err;
	unsigned int val[OTP_MAX_WORD_LEN];

	/* read all the OTP memory */
	err = read_otp_memory(0, &val[0], OTP_MAX_WORD_LEN);
	if (err) {
		shell_fprintf(shell, SHELL_ERROR, "FAILED reading otp memory......\n");
		return -ENOEXEC;
	}

	shell_fprintf(shell, SHELL_INFO, "Checking OTP PROTECT Region......\n");
	ret = check_protection(&val[0], REGION_PROTECT, REGION_PROTECT + 1,
							REGION_PROTECT + 2, REGION_PROTECT + 3);

	disp_location_status(shell, "OTP", ret);
	disp_fields_status(shell, val[REGION_DEFAULTS]);

	return 0;
}

static int nrf_wifi_radio_test_otp_read_params(const struct shell *shell,
					size_t argc,
					char  *argv[])
{
	unsigned int val[OTP_MAX_WORD_LEN];
	unsigned int ret, err;

	err = read_otp_memory(0, &val[0], OTP_MAX_WORD_LEN);
	if (err) {
		shell_fprintf(shell, SHELL_ERROR, "FAILED reading otp memory......\n");
		return -ENOEXEC;
	}

	ret = check_protection(&val[0], REGION_PROTECT + 0, REGION_PROTECT + 1,
							REGION_PROTECT + 2, REGION_PROTECT + 3);
	disp_location_status(shell, "OTP", ret);

	shell_fprintf(shell, SHELL_INFO, "REGION_PROTECT0 = 0x%08x\n", val[REGION_PROTECT + 0]);
	shell_fprintf(shell, SHELL_INFO, "REGION_PROTECT1 = 0x%08x\n", val[REGION_PROTECT + 1]);
	shell_fprintf(shell, SHELL_INFO, "REGION_PROTECT2 = 0x%08x\n", val[REGION_PROTECT + 2]);
	shell_fprintf(shell, SHELL_INFO, "REGION_PROTECT3 = 0x%08x\n", val[REGION_PROTECT + 3]);
	shell_fprintf(shell, SHELL_INFO, "\n");

	shell_fprintf(shell, SHELL_INFO, "PRODTEST_FT_PROGVERSION1 = 0x%08x\n",
		val[PRODTEST_FT_PROGVERSION1]);
	shell_fprintf(shell, SHELL_INFO, "PRODTEST_FT_PROGVERSION2 = 0x%08x\n",
		val[PRODTEST_FT_PROGVERSION2]);
	shell_fprintf(shell, SHELL_INFO, "\n");

	shell_fprintf(shell, SHELL_INFO, "PRODTEST_TRIM0: Reg0 = 0x%08x\n", val[PRODTEST_TRIM0]);
	shell_fprintf(shell, SHELL_INFO, "PRODTEST_TRIM1: Reg1 = 0x%08x\n", val[PRODTEST_TRIM1]);
	shell_fprintf(shell, SHELL_INFO, "PRODTEST_TRIM2: Reg2 = 0x%08x\n", val[PRODTEST_TRIM2]);
	shell_fprintf(shell, SHELL_INFO, "PRODTEST_TRIM3: Reg3 = 0x%08x\n", val[PRODTEST_TRIM3]);
	shell_fprintf(shell, SHELL_INFO, "PRODTEST_TRIM4: Reg4 = 0x%08x\n", val[PRODTEST_TRIM4]);
	shell_fprintf(shell, SHELL_INFO, "PRODTEST_TRIM5: Reg5 = 0x%08x\n", val[PRODTEST_TRIM5]);
	shell_fprintf(shell, SHELL_INFO, "PRODTEST_TRIM6: Reg6 = 0x%08x\n", val[PRODTEST_TRIM6]);
	shell_fprintf(shell, SHELL_INFO, "PRODTEST_TRIM7: Reg7 = 0x%08x\n", val[PRODTEST_TRIM7]);
	shell_fprintf(shell, SHELL_INFO, "PRODTEST_TRIM8: Reg8 = 0x%08x\n", val[PRODTEST_TRIM8]);
	shell_fprintf(shell, SHELL_INFO, "PRODTEST_TRIM9: Reg9 = 0x%08x\n", val[PRODTEST_TRIM9]);
	shell_fprintf(shell, SHELL_INFO, "PRODTEST_TRIM10: Reg10 = 0x%08x\n",
		val[PRODTEST_TRIM10]);
	shell_fprintf(shell, SHELL_INFO, "PRODTEST_TRIM11: Reg11 = 0x%08x\n",
		val[PRODTEST_TRIM11]);
	shell_fprintf(shell, SHELL_INFO, "PRODTEST_TRIM12: Reg12 = 0x%08x\n",
		val[PRODTEST_TRIM12]);
	shell_fprintf(shell, SHELL_INFO, "PRODTEST_TRIM13: Reg13 = 0x%08x\n",
		val[PRODTEST_TRIM13]);
	shell_fprintf(shell, SHELL_INFO, "PRODTEST_TRIM14: Reg14 = 0x%08x\n",
		val[PRODTEST_TRIM14]);
	shell_fprintf(shell, SHELL_INFO, "\n");

	shell_fprintf(shell, SHELL_INFO, "PRODCTRL.DISABLE5GHZ = 0x%08x\n",
		val[PRODCTRL_DISABLE5GHZ]);
	shell_fprintf(shell, SHELL_INFO, "\n");

	shell_fprintf(shell, SHELL_INFO, "MAC0: Reg0 = 0x%08x\n", val[MAC0_ADDR]);
	shell_fprintf(shell, SHELL_INFO, "MAC0: Reg1 = 0x%08x\n", val[MAC0_ADDR + 1]);
	shell_fprintf(shell, SHELL_INFO, "MAC0 Addr  = %02x:%02x:%02x:%02x:%02x:%02x\n",
		(uint8_t)(val[MAC0_ADDR]), (uint8_t)(val[MAC0_ADDR] >> 8),
		(uint8_t)(val[MAC0_ADDR] >> 16), (uint8_t)(val[MAC0_ADDR] >> 24),
		(uint8_t)(val[MAC0_ADDR + 1]), (uint8_t)(val[MAC0_ADDR + 1] >> 8));
	shell_fprintf(shell, SHELL_INFO, "\n");

	shell_fprintf(shell, SHELL_INFO, "MAC1 : Reg0 = 0x%08x\n", val[MAC1_ADDR]);
	shell_fprintf(shell, SHELL_INFO, "MAC1 : Reg1 = 0x%08x\n", val[MAC1_ADDR + 1]);
	shell_fprintf(shell, SHELL_INFO, "MAC1 Addr   = %02x:%02x:%02x:%02x:%02x:%02x\n",
		(uint8_t)(val[MAC1_ADDR]), (uint8_t)(val[MAC1_ADDR] >> 8),
		(uint8_t)(val[MAC1_ADDR] >> 16), (uint8_t)(val[MAC1_ADDR] >> 24),
		(uint8_t)(val[MAC1_ADDR + 1]), (uint8_t)(val[MAC1_ADDR + 1] >> 8));
	shell_fprintf(shell, SHELL_INFO, "\n");
	shell_fprintf(shell, SHELL_INFO, "CALIB_XO = 0x%02x\n", val[CALIB_XO] & 0xFF);


	shell_fprintf(shell, SHELL_INFO, "REGION_DEFAULTS = 0x%08x\n",
		val[REGION_DEFAULTS]);
	shell_fprintf(shell, SHELL_INFO, "\n");

	return 0;
}

static int nrf_wifi_radio_test_otp_write_params(const struct shell *shell,
					size_t argc,
					char  *argv[])
{
	unsigned int field;
	unsigned int write_val[20];
	unsigned int val[OTP_MAX_WORD_LEN];
	unsigned int ret, err;
	int status = 0;

	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "invalid # of args : %d\n", argc);
		return -ENOEXEC;
	}

	field  = strtoul(argv[1], NULL, 0);
	/* Align to 32-bit word address */
	field >>= 2;

	if (field < REGION_PROTECT) {
		shell_fprintf(shell, SHELL_ERROR, "INVALID Address 0x%x......\n", field << 2);
		return -ENOEXEC;
	}

	err = read_otp_memory(REGION_PROTECT, &val[REGION_PROTECT], 4);
	if (err) {
		shell_fprintf(shell, SHELL_ERROR, "FAILED reading otp memory......\n");
		return -ENOEXEC;
	}

	ret = check_protection(&val[0], REGION_PROTECT, REGION_PROTECT + 1,
						REGION_PROTECT + 2, REGION_PROTECT + 3);
	disp_location_status(shell, "OTP", ret);
	if (ret != OTP_ENABLE_PATTERN && ret != OTP_FRESH_FROM_FAB) {
		shell_fprintf(shell, SHELL_ERROR, "USER Region is not Writeable\n");
		return -ENOEXEC;
	}

	switch (field) {
	case REGION_PROTECT:
		if (argc != 3) {
			shell_fprintf(shell, SHELL_ERROR,
					"invalid # of args for REGION_PROTECT (expected 3) : %d\n",
					argc);
			return -ENOEXEC;
		}
		write_val[0]  = strtoul(argv[2], NULL, 0);
		/* All consecutive 4 words of the REGION_PROTECT are written */
		write_otp_memory(REGION_PROTECT, &write_val[0]);
		break;
	case MAC0_ADDR:
	case MAC1_ADDR:
		if (argc != 4) {
			shell_fprintf(shell, SHELL_ERROR,
				   "invalid # of args for MAC ADDR write (expected 4) : %d\n",
					argc);
			return -ENOEXEC;
		}
		write_val[0] = strtoul(argv[2], NULL, 0);
		write_val[1] = strtoul(argv[3], NULL, 0) & 0xFFFF;

		if (!nrf_wifi_utils_is_mac_addr_valid((const char *)write_val)) {
			shell_fprintf(shell,
				      SHELL_ERROR,
				      "Invalid MAC address. MAC address cannot be all 0's, broadcast or multicast address\n");
			return -ENOEXEC;
		}

		status = write_otp_memory(field, &write_val[0]);
		break;
	case CALIB_XO:
	case REGION_DEFAULTS:
		if (argc != 3) {
			shell_fprintf(shell, SHELL_ERROR,
			  "invalid # of args for field %d (expected 3) : %d\n", field, argc);
			return -ENOEXEC;
		}
		write_val[0]  = strtoul(argv[2], NULL, 0);
		status = write_otp_memory(field, &write_val[0]);
		break;
	case QSPI_KEY:
		if (argc != 6) {
			shell_fprintf(shell, SHELL_ERROR,
					"invalid # of args for QSPI_KEY (expected 6) : %d\n", argc);
			return -ENOEXEC;
		}
		write_val[0]  = strtoul(argv[2], NULL, 0);
		write_val[1]  = strtoul(argv[3], NULL, 0);
		write_val[2]  = strtoul(argv[4], NULL, 0);
		write_val[3]  = strtoul(argv[5], NULL, 0);
		/* All consecutive 4 words of the qspi keys are written now */
		status = write_otp_memory(QSPI_KEY, &write_val[0]);
		break;
	default:
		shell_fprintf(shell, SHELL_ERROR, "unsupported field %d\n", field);
		return -ENOEXEC;
	}

	if (!status) {
		shell_fprintf(shell, SHELL_INFO, "Finished Writing OTP params\n");
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	nrf_wifi_radio_otp_subcmds,
	SHELL_CMD_ARG(otp_get_status,
			  NULL,
			  "Read OTP status",
			  nrf_wifi_radio_test_otp_get_status,
			  1,
			  0),
	SHELL_CMD_ARG(otp_read_params,
			  NULL,
			  "Read User region status and information on programmed fields",
			  nrf_wifi_radio_test_otp_read_params,
			  1,
			  0),
	SHELL_CMD_ARG(otp_write_params,
			  NULL,
			  "Write OTP Params\n"
			  "otp_write_params <addr offset> [arg1] [arg2]...[argN]",
			  nrf_wifi_radio_test_otp_write_params,
			  2,
			  16),
	SHELL_SUBCMD_SET_END);


SHELL_CMD_REGISTER(wifi_radio_ficr_prog,
		   &nrf_wifi_radio_otp_subcmds,
		   "nRF Wi-Fi radio FICR commands",
		   NULL);


static int nrf_wifi_radio_otp_shell_init(void)
{

	return 0;
}


SYS_INIT(nrf_wifi_radio_otp_shell_init,
	 APPLICATION,
	 CONFIG_APPLICATION_INIT_PRIORITY);
