#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources_ifdef(CONFIG_NFC_T4T_NDEF_FILE ndef_file.c)
zephyr_sources_ifdef(CONFIG_NFC_T4T_ISODEP isodep.c)
zephyr_sources_ifdef(CONFIG_NFC_T4T_APDU apdu.c)
zephyr_sources_ifdef(CONFIG_NFC_T4T_CC_FILE
		     cc_file.c
		     tlv_block.c)
zephyr_sources_ifdef(CONFIG_NFC_T4T_HL_PROCEDURE hl_procedure.c)
