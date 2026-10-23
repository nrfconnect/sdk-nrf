#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_amend()

zephyr_library_sources_ifdef(CONFIG_AUDIO_TAC5112 tac5112.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_TAC5112_EMUL tac5112_emul.c)
