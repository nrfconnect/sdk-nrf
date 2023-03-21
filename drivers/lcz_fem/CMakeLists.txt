# Copyright (c) 2023 Laird Connectivity.
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(CONFIG_LCZ_FEM)
    zephyr_library()
    zephyr_library_sources(fem_gpio.c)

    if(CONFIG_BT OR CONFIG_NRF_802154_SL)
        zephyr_library_sources(fem_power.c)
        zephyr_library_include_directories(power_table)

        #
        # Generate power table file name based on type of antenna and region
        #

        # BL5340PA is currently the only option.
        set(PT_PATH "power_table/bl5340pa")

        if(CONFIG_LCZ_FEM_INTERNAL_ANTENNA)
            set(PT_TYPE "internal")
        else()
            set(PT_TYPE "external")
        endif()

        if(CONFIG_LCZ_FEM_REGION EQUAL 0)
            set(PT_REGION "fcc_ic")
        else()
            set(PT_REGION "rcm")
        endif()

        zephyr_library_sources(${PT_PATH}/${PT_REGION}_${PT_TYPE}.c)
    endif() # BT or 802.15.4
endif() # FEM_DRIVER
