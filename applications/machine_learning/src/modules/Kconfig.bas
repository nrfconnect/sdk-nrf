#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

menuconfig ML_APP_BAS_ENABLE
	bool "Enable GATT Battery Service"
	help
	  This option enables GATT Battery Service application
	  module.

if ML_APP_BAS_ENABLE

module = ML_APP_BAS
module-str = battery service
source "subsys/logging/Kconfig.template.log_config"

endif
