#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Set the CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM defined in smp_transfer/Kconfig
# to be equal to SB_CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM
set_property(TARGET smp_transfer APPEND_STRING PROPERTY CONFIG "CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM=${SB_CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM}\n")
