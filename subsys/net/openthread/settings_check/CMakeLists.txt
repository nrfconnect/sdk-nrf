#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This script checks if NVS settings backend is used with nRF54L SoC target and prints
# the warning that the recommended backend is ZMS due to the RRAM design.

if((CONFIG_SOC_SERIES_NRF54L OR CONFIG_SOC_SERIES_NRF54H) AND CONFIG_SETTINGS_NVS)
    message(WARNING " ###################################################################################\n"
                    " #                                                                                 #\n"
                    " # Your application uses NVS backend for the settings storage that is not          #\n"
                    " # optimal solution for the nRF54L or the nRF54H SoC family and it may negatively  #\n"
                    " # impact the non-volatile RRAM or MRAM life-time.                                 #\n"
                    " #                                                                                 #\n"
                    " # The recommended settings backend for nRF54L or nRF54H SoC family is ZMS.        #\n"
                    " # You can enable it by selecting CONFIG_SETTINGS_ZMS=y and CONFIG_ZMS=y.          #\n"
                    " # Please note that the other Kconfig options related to NVS like cache            #\n"
                    " # or block size will require alignment on the ZMS side as well.                   #\n"
                    " #                                                                                 #\n"
                    " ###################################################################################\n")
endif()
