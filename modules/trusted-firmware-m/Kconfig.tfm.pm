#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

menu "TF-M memory regions"

config PM_PARTITION_SIZE_TFM_SRAM
	hex
	prompt "Memory reserved for TFM_RAM" if !TFM_PROFILE_TYPE_MINIMAL
	default 0x8000 if TFM_PROFILE_TYPE_MINIMAL
	default 0x18000 if SOC_SERIES_NRF91X && TFM_REGRESSION_S
	# It has been observed for 54L that when Matter is enabled, then
	# assigning 0x16000 of RAM to TFM will not leave enough RAM for
	# Matter. So we use 0x13000 of RAM on 54L.
	default 0x13000 if SOC_SERIES_NRF54LX
	default 0x16000 if SOC_SERIES_NRF91X
	default 0x30000
	help
	  Memory set aside for the TFM_SRAM partition.

config PM_PARTITION_SIZE_TFM
	hex
	prompt  "Memory reserved for TFM" if !TFM_PROFILE_TYPE_MINIMAL
	default 0xFE00 if TFM_PROFILE_TYPE_MINIMAL && TFM_CMAKE_BUILD_TYPE_DEBUG && \
		BOOTLOADER_MCUBOOT
	default 0x10000 if TFM_PROFILE_TYPE_MINIMAL && TFM_CMAKE_BUILD_TYPE_DEBUG
	default 0x7E00 if TFM_PROFILE_TYPE_MINIMAL && BOOTLOADER_MCUBOOT
	default 0x8000 if TFM_PROFILE_TYPE_MINIMAL
	# NCSDK-13503: Temporarily bump size while regressions are being fixed
	default 0x60000 if TFM_REGRESSION_S
	default 0x4FE00 if TFM_CMAKE_BUILD_TYPE_DEBUG && BOOTLOADER_MCUBOOT
	default 0x50000 if TFM_CMAKE_BUILD_TYPE_DEBUG
	default 0x3FE00 if BOOTLOADER_MCUBOOT
	default 0x40000
	help
	  Memory set aside for the TFM partition. This size is fixed if
	  TFM_PROFILE_TYPE_MINIMAL is set. This is because no modification of the features
	  supported by TF-M can be performed when TFM_PROFILE_TYPE_MINIMAL is enabled.

config PM_PARTITION_SIZE_TFM_PROTECTED_STORAGE
	hex "Memory reserved for TFM Protected Storage"
	default 0x4000 if TFM_PARTITION_PROTECTED_STORAGE
	default 0
	help
	  Memory set aside for the TFM Protected Storage (PS) partition.

config PM_PARTITION_SIZE_TFM_INTERNAL_TRUSTED_STORAGE
	hex "Memory reserved for TFM Internal Trusted Storage"
	default 0x2000 if TFM_PARTITION_INTERNAL_TRUSTED_STORAGE
	default 0
	help
	  Memory set aside for the TFM Internal Trusted Storage (ITS) partition

config PM_PARTITION_SIZE_TFM_OTP_NV_COUNTERS
	hex "Memory reserved for TFM OTP / Non-Volatile Counters"
	default 0x2000 if !TFM_PROFILE_TYPE_MINIMAL
	default 0
	help
	  Memory set aside for the OTP / Non-Volatile (NV) Counters partition

config PM_PARTITION_TFM_STORAGE
	bool
	default y if PM_PARTITION_SIZE_TFM_PROTECTED_STORAGE != 0
	default y if PM_PARTITION_SIZE_TFM_INTERNAL_TRUSTED_STORAGE != 0
	default y if PM_PARTITION_SIZE_TFM_OTP_NV_COUNTERS != 0

endmenu
