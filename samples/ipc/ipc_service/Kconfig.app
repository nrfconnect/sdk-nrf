#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

config APP_IPC_SERVICE_SEND_INTERVAL
	int "Ipc service sending interval [us]"
	default 30 if BOARD_NRF5340DK_NRF5340_CPUNET
	default 70
	help
	  Time in micro seconds between sending subsequent data packages over
	  IPC service. Since kernel timeout has 1 ms resolution, the value is
	  rounded down. If value of this option is lower than 1000 us, busy
	  wait is used instead of sleep.

config APP_IPC_SERVICE_MESSAGE_LEN
	int "Length of single IPC message in bytes"
	default 100
