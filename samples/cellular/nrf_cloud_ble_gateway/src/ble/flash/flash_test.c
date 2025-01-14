/* Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include <string.h>

#if CONFIG_SPI_NOR || DT_NODE_HAS_STATUS(DT_INST(0, jedec_spi_nor), okay)
#define FLASH_NAME "JEDEC SPI-NOR"
#elif CONFIG_NORDIC_QSPI_NOR || DT_NODE_HAS_STATUS(DT_INST(0, nordic_qspi_nor), okay)
#define FLASH_NAME "JEDEC QSPI-NOR"
#else
#error Unsupported flash driver
#endif

#define FLASH_TEST_REGION_OFFSET 0xff000
#define FLASH_SECTOR_SIZE 4096

#define MEMCTRL_NODE DT_NODELABEL(ext_mem_ctrl)

#define MEMCTRL_GPIO_CTRL  DT_GPIO_CTLR(MEMCTRL_NODE, gpios)
#define MEMCTRL_GPIO_PIN   DT_GPIO_PIN(MEMCTRL_NODE, gpios)

/**
 * Set the external mem control pin to high to
 * enable access to the external memory chip.
 */
static int flash_test_init(void)
{
	const struct device *port;
	int err;

	printk("%s\n", __func__);

	port = DEVICE_DT_GET(MEMCTRL_GPIO_CTRL);
	if (!port) {
		printk("memctrl port not found: %d\n", errno);
		return -EIO;
	}

	err = gpio_pin_configure(port, MEMCTRL_GPIO_PIN, GPIO_OUTPUT_HIGH);
	if (err) {
		printk("error on gpio_pin_configure(): %d\n", err);
		return err;
	}

	printk("init complete\n");
	return err;
}

SYS_INIT(flash_test_init, POST_KERNEL, CONFIG_SPI_NOR_INIT_PRIORITY);

int flash_test(void)
{
	const uint8_t expected[] = {
		0x55, 0xaa, 0x66, 0x99, 0xa5, 0x5a, 0x69, 0x96,
		0xa5, 0x5a, 0x96, 0x69, 0x5a, 0xa5, 0x4b, 0xb4
	};
	const size_t len = sizeof(expected);
	uint8_t buf[sizeof(expected)];
	const struct device *flash_dev;
	int rc;
	int ret = 0;

	printk("\n" FLASH_NAME " SPI flash testing\n");
	printk("==========================\n");

#if CONFIG_SPI_NOR || DT_NODE_HAS_STATUS(DT_INST(0, jedec_spi_nor), okay)
	flash_dev = DEVICE_DT_GET_ONE(jedec_spi_nor);
#elif CONFIG_NORDIC_QSPI_NOR || DT_NODE_HAS_STATUS(DT_INST(0, nordic_qspi_nor), okay)
	flash_dev = DEVICE_DT_GET_ONE(jedec_qspi_nor);
#endif

	if (!flash_dev) {
		printk("SPI flash driver %s was not found!\n", FLASH_NAME);
		ret = -EIO;
		goto error;
	}

	printk("\nTest 1: Flash erase: ");

	rc = flash_erase(flash_dev, FLASH_TEST_REGION_OFFSET,
			 FLASH_SECTOR_SIZE);
	if (rc != 0) {
		printk("FAIL - %d\n", rc);
	} else {
		printk("PASS\n");
	}

	printk("\nTest 2: Flash write %u bytes: ", len);

	rc = flash_write(flash_dev, FLASH_TEST_REGION_OFFSET, expected, len);
	if (rc != 0) {
		printk("FAIL - %d\n", rc);
		ret = -EIO;
		goto error;
	} else {
		printk("PASS\n");
	}

	printk("\nTest 3: Flash read %u bytes: ", len);
	memset(buf, 0, len);
	rc = flash_read(flash_dev, FLASH_TEST_REGION_OFFSET, buf, len);
	if (rc != 0) {
		printk("FAIL - %d\n", rc);
		ret = -EIO;
		goto error;
	} else {
		printk("PASS\n");
	}

	printk("\nTest 4: Compare %u bytes: ", len);
	if (memcmp(expected, buf, len) != 0) {
		const uint8_t *wp = expected;
		const uint8_t *rp = buf;
		const uint8_t *rpe = rp + len;

		ret = -EFAULT;
		printk("FAIL - %d\n", ret);
		while (rp < rpe) {
			printk("%08x wrote %02x read %02x %s\n",
			       (uint32_t)(FLASH_TEST_REGION_OFFSET + (rp - buf)),
			       *wp, *rp, (*rp == *wp) ? "match" : "MISMATCH");
			++rp;
			++wp;
		}
		goto error;
	} else {
		printk("PASS\n");
	}
	return 0;

error:
	return ret;
}
