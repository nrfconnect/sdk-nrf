#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# bl_storage.c needs an nrfx storage backend (NVMC or RRAMC) for its OTP/UICR
# accesses, and nrf_nv_counters.c is a shim layered on top of bl_storage.c, so it
# needs the same backend. On SoCs whose storage peripheral is secure-only (e.g.
# the nRF54L RRAMC) that backend is not selected for the non-secure image, where
# the application instead reaches boot storage through TF-M services. Only build
# the library when a backend is available so the non-secure image does not pull
# in the (uncompilable) secure-only driver.
if(CONFIG_NRFX_NVMC OR CONFIG_NRFX_RRAMC)
  zephyr_library()
  zephyr_library_sources(bl_storage.c)

  if(CONFIG_TRUSTED_EXECUTION_NONSECURE)
    zephyr_get(tfm_bl_storage_addr SYSBUILD LOCAL VAR TFM_BL_STORAGE_ADDR)

    if(tfm_bl_storage_addr AND "${tfm_bl_storage_addr}" GREATER "0")
      zephyr_library_compile_definitions(TFM_BL_STORAGE_ADDR=${tfm_bl_storage_addr})
    endif()
  endif()

  if(CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION)
    # NB: This option can only be enabled for the MCUBoot image
    zephyr_library_sources(nrf_nv_counters.c)
  endif()
elseif(CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION)
  message(WARNING
    "CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION requires the bl_storage backend, but "
    "neither CONFIG_NRFX_NVMC nor CONFIG_NRFX_RRAMC is enabled."
  )
endif()
