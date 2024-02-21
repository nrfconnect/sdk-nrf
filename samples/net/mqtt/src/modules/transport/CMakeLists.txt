#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/transport.c)

# Add Client ID helper library
add_subdirectory(client_id)

# Add credentials provision library if the Modem key Management API is enabled.
# The library provisions credentials placed in the src/transport/credentials/ folder to
# the nRF91 modem.
add_subdirectory_ifdef(CONFIG_MODEM_KEY_MGMT credentials_provision)
