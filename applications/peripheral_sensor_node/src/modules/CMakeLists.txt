#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE
  app_control.c
  nsms.c
  env_history.c
  led_state.c
)

target_sources_ifdef(CONFIG_APP_SENSORS app PRIVATE sensors.c)
