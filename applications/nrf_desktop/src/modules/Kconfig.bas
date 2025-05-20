#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

menu "BLE BAS Service"

config DESKTOP_BAS_ENABLE
	bool "Enable GATT Battery Service"
	depends on DESKTOP_BT_PERIPHERAL
	help
	  This option enables GATT Battery Service application
	  module.

if DESKTOP_BAS_ENABLE

module = DESKTOP_BAS
module-str = battery service
source "subsys/logging/Kconfig.template.log_config"

endif

endmenu
