#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library_amend()
zephyr_library_sources_ifdef(CONFIG_ENTROPY_CC3XX entropy_cc310.c)

# Link with the nrf_cc3xx_platform library if the following is met:
# -nRF52840 device
# -nRF9160/nRF53_CPUAPP device that is not using SPM
# -nRF9150 device that is using SPM and in a secure image
# -nRF53_CPUAPP device that is using SPM and in a secure image
#  (CONFIG_SPM is not defined in a secure image)
if (CONFIG_SOC_NRF52840 OR
   (CONFIG_SOC_NRF9160 AND (NOT CONFIG_SPM)) OR
   (CONFIG_SOC_NRF5340_CPUAPP AND (NOT CONFIG_SPM)))
  zephyr_link_libraries_ifdef(CONFIG_ENTROPY_CC3XX platform_cc3xx)
endif ()
