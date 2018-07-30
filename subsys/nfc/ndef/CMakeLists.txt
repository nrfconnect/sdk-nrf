#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_NFC_NDEF_MSG_ENABLED nfc_ndef_msg.c)
zephyr_library_sources_ifdef(CONFIG_NFC_NDEF_RECORD_ENABLED nfc_ndef_record.c)
zephyr_library_sources_ifdef(CONFIG_NFC_NDEF_TEXT_RECORD_ENABLED nfc_text_rec.c)
zephyr_library_sources_ifdef(CONFIG_NFC_NDEF_URI_MSG_ENABLED nfc_uri_msg.c)
zephyr_library_sources_ifdef(CONFIG_NFC_NDEF_URI_REC_ENABLED nfc_uri_rec.c)
