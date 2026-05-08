#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

menuconfig BT_HIDS_SCI
	bool "HID Shorter Connection Intervals (SCI) for HIDS service"
	depends on BT_SHORTER_CONNECTION_INTERVALS
	select EXPERIMENTAL

if BT_HIDS_SCI

config BT_HIDS_SCI_LOW_POWER_MODE
	bool "HID SCI low power mode"
	default y
	help
	  Enable HID Shorter Connection Intervals (SCI) low power mode.

# The default values below are based on the recommended values from
# the HID over GATT Profile specification (chapter 7.4.1), except
# supervision timeout defaults: smallest HCI-legal value (10 = 100 ms,
# Core Vol 4 Part E) that still satisfies Link Layer §4.5.2 for each
# mode's default interval_max, subrate_max, and max_latency.

# Default mode
MODE = DEFAULT
MODE_PRETTY = Default
sci_interval_min_default = 60
sci_interval_max_default = 120
sci_subrate_min_default = 1
sci_subrate_max_default = 4
sci_max_peripheral_latency_default = 0
sci_continuation_number_default = 0
sci_supervision_timeout_10ms_default = 13
rsource "Kconfig.template.hids.sci_mode"

# Fast mode
MODE = FAST
MODE_PRETTY = Fast
sci_interval_min_default = 10
sci_interval_max_default = 40
sci_subrate_min_default = 1
sci_subrate_max_default = 4
sci_max_peripheral_latency_default = 0
sci_continuation_number_default = 3
sci_supervision_timeout_10ms_default = 10
rsource "Kconfig.template.hids.sci_mode"

if BT_HIDS_SCI_LOW_POWER_MODE

# Low power mode
MODE = LOW_POWER
MODE_PRETTY = Low_Power
sci_interval_min_default = 60
sci_interval_max_default = 120
sci_subrate_min_default = 1
sci_subrate_max_default = 4
sci_max_peripheral_latency_default = 100
sci_continuation_number_default = 0
sci_supervision_timeout_10ms_default = 1213
rsource "Kconfig.template.hids.sci_mode"

endif

# Full range mode
MODE = FULL_RANGE
MODE_PRETTY = Full_Range
sci_interval_min_default = 10
sci_interval_max_default = 120
sci_subrate_min_default = 1
sci_subrate_max_default = 4
sci_max_peripheral_latency_default = 0
sci_continuation_number_default = 1
sci_supervision_timeout_10ms_default = 13
rsource "Kconfig.template.hids.sci_mode"

endif
