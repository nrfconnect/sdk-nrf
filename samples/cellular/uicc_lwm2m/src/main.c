/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <ctype.h>

#include <zephyr/kernel.h>
#include <modem/nrf_modem_lib.h>
#include <modem/uicc_lwm2m.h>
#include <nrf_modem_at.h>

uint8_t buffer[UICC_RECORD_BUFFER_MAX];

static void printk_hexdump(char *data, int len)
{
	static const char hexchars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
					   'a', 'b', 'c', 'd', 'e', 'f' };
	int pos = 0;

	while (pos < len) {
		char hexbuf[256];
		char *cur = hexbuf;
		int i;

		/* Dump offset */
		cur += snprintf(cur, 20, "%04x  ", pos);

		/* Dump bytes as hex */
		for (i = 0; i < 16 && pos + i < len; i++) {
			*cur++ = hexchars[(data[pos + i] & 0xf0) >> 4];
			*cur++ = hexchars[data[pos + i] & 0x0f];
			*cur++ = ' ';
			if (i == 7) {
				*cur++ = ' ';
			}
		}

		while (cur < hexbuf + 58) {
			/* Fill it up with space to ascii column */
			*cur++ = ' ';
		}

		/* Dump bytes as text */
		for (i = 0; i < 16 && pos + i < len; i++) {
			if (isprint((int)data[pos + i])) {
				*cur++ = data[pos + i];
			} else {
				*cur++ = '.';
			}
			if (i == 7) {
				*cur++ = ' ';
			}
		}

		pos += i;
		*cur = 0;

		printk("%s\n", hexbuf);
	}
}

int main(void)
{
	int ret;

	printk("UICC LwM2M sample started\n");

	ret = nrf_modem_lib_init();
	if (ret) {
		printk("Modem library initialization failed, error: %d\n", ret);
		return -1;
	}

	ret = nrf_modem_at_printf("AT+CFUN=41");
	if (ret) {
		printk("Activate UICC failed, error: %d\n", ret);
		return -1;
	}

	ret = uicc_lwm2m_bootstrap_read(buffer, sizeof(buffer));
	if (ret > 0) {
		printk("LwM2M bootstrap data found, length: %d\n\n", ret);
		printk_hexdump(buffer, ret);
	} else {
		printk("Failed to read data, error: %d\n", -ret);
	}

	ret = nrf_modem_lib_shutdown();
	if (ret) {
		printk("Modem library shutdown failed, error: %d\n", ret);
		return -1;
	}

	return 0;
}
