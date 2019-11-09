#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
zephyr_library_amend()
zephyr_library_sources_if_kconfig(entropy_cc310.c)

# Link with the nrf_cc310 platform library if the following is met:
# -nRF52840 device
# -nRF9160 device that is not using SPM
# -nRF9150 device that is using SPM and in a secure image
#  (CONFIG_SPM is not defined in a secure image)
if (CONFIG_SOC_NRF52840 OR (CONFIG_SOC_NRF9160 AND (NOT CONFIG_SPM)))
  zephyr_link_libraries_ifdef(CONFIG_ENTROPY_CC310 platform_cc310)
endif ()
