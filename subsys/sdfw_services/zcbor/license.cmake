#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

string(TIMESTAMP current_year "%Y" UTC)
set(LICENSE "\
/*
 * Copyright (c) ${current_year} Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

")

foreach(file ${FILES})
  file(READ ${file} SOURCE_CONTENT)
  file(WRITE ${file} "${LICENSE}${SOURCE_CONTENT}")
endforeach()
