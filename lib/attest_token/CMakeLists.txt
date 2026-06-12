#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(attest_token.c)
zephyr_library_sources_ifdef(CONFIG_ATTEST_TOKEN_BACKEND_MODEM attest_token_modem.c)
zephyr_library_sources_ifdef(CONFIG_ATTEST_TOKEN_BACKEND_APP attest_token_app.c)
