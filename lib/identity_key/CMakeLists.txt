#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(identity_key.c)

if (CONFIG_IDENTITY_KEY_DUMMY)
  message(WARNING "
      ----------------------------------------------------------
      --- WARNING: A dummy identity key will be written to   ---
      --- the KMU. DO NOT USE THIS KEY IN PRODUCTION!        ---
      ----------------------------------------------------------")
endif()
