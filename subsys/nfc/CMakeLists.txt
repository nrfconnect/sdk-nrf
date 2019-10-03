#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if (CONFIG_NFC_NDEF OR
    CONFIG_NFC_NDEF_PARSER)
        add_subdirectory(ndef)
endif()

add_subdirectory_ifdef(CONFIG_NFC_T2T_PARSER t2t)

if (CONFIG_NFC_T4T_ISODEP OR
    CONFIG_NFC_T4T_APDU OR
    CONFIG_NFC_T4T_CC_FILE OR
    CONFIG_NFC_T4T_HL_PROCEDURE)
	add_subdirectory(t4t)
endif()


if (CONFIG_NFC_T2T_NRFXLIB OR
    CONFIG_NFC_T4T_NRFXLIB)
	add_subdirectory(lib)
endif()

if (CONFIG_NFC_TNEP_TAG OR
    CONFIG_NFC_TNEP_POLLER)
	add_subdirectory(tnep)
endif()
