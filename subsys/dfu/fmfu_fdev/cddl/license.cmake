#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(LICENSE "\
/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

")

foreach (file ${FILES})
  file(READ ${file} SOURCE_CONTENT)
  file(WRITE ${file} "${LICENSE}${SOURCE_CONTENT}")
endforeach()
