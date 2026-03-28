#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_sources_ifdef(CONFIG_PPI_SEQ ppi_seq.c)
zephyr_library_sources_ifdef(CONFIG_PPI_SEQ_I2C_SPI ppi_seq_i2c_spi.c)
