#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

add_subdirectory_ifdef(CONFIG_NFC_NDEF ndef)
add_subdirectory_ifdef(CONFIG_NFC_T2T_PARSER t2t)
