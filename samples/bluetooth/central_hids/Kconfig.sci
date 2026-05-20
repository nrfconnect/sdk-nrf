#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

menu "Central HIDS SCI default connection rate parameters"

# Do not confuse the central default connection rate parameters
# with the HIDS SCI default mode parameters.
# Central default rate parameters (bt_conn_le_conn_rate_set_defaults) apply to future
# connections as the host's default window
# HID SCI default (and other) mode link parameters are chosen in procedures
# with the peripheral and typically differ from these local defaults.

config SAMPLE_CENTRAL_HIDS_SCI_INTERVAL_MIN_125US
	int "Minimum connection interval (125 microsecond units)"
	default 7
	range 3 32000
	help
	  Minimum connection interval in 125 microsecond units (LE connection interval
	  quantum), matching struct bt_conn_le_conn_rate_param.interval_min_125us.

config SAMPLE_CENTRAL_HIDS_SCI_INTERVAL_MAX_125US
	int "Maximum connection interval (125 microsecond units)"
	default 120
	range SAMPLE_CENTRAL_HIDS_SCI_INTERVAL_MIN_125US 32000
	help
	  Maximum connection interval in 125 microsecond units (LE connection interval
	  quantum), matching struct bt_conn_le_conn_rate_param.interval_max_125us.

config SAMPLE_CENTRAL_HIDS_SCI_SUBRATE_MIN
	int "Subrate factor minimum"
	default 1
	range 1 500
	help
	  Minimum subrate factor, matching struct bt_conn_le_conn_rate_param.subrate_min.

config SAMPLE_CENTRAL_HIDS_SCI_SUBRATE_MAX
	int "Subrate factor maximum"
	default 4
	range SAMPLE_CENTRAL_HIDS_SCI_SUBRATE_MIN 500
	help
	  Maximum subrate factor, matching struct bt_conn_le_conn_rate_param.subrate_max.

config SAMPLE_CENTRAL_HIDS_SCI_MAX_LATENCY
	int "Maximum peripheral latency"
	default 100
	range 0 499
	help
	  Maximum peripheral latency, matching struct bt_conn_le_conn_rate_param.max_latency.

config SAMPLE_CENTRAL_HIDS_SCI_CONTINUATION_NUM
	int "Continuation number"
	default 0
	range 0 499
	help
	  Continuation number, matching struct bt_conn_le_conn_rate_param.continuation_number.

config SAMPLE_CENTRAL_HIDS_SCI_SUPERVISION_TIMEOUT_10MS
	int "Supervision timeout (units of 10 ms)"
	default 3200
	range 10 3200
	help
	  Link supervision timeout, matching struct
	  bt_conn_le_conn_rate_param.supervision_timeout_10ms.

endmenu
