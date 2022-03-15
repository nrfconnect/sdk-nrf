#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(fast_pair)

zephyr_library_include_directories(include)

zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_ADVERTISING		fp_advertising.c)
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_CRYPTO_MBEDTLS		fp_crypto_mbedtls.c)
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_CRYPTO_TINYCRYPT	fp_crypto_tinycrypt.c)
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_GATT_SERVICE		fp_gatt_service.c)
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_REGISTRATION_DATA	fp_registration_data.c)
