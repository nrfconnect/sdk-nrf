#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

menu "BLE BAS Service"

config DESKTOP_BAS_ENABLE
	bool
	help
	  This option enables battery service.

if DESKTOP_BAS_ENABLE

module = DESKTOP_BAS
module-str = battery service
source "subsys/logging/Kconfig.template.log_config"

endif

endmenu
