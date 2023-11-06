#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT DEFINED FP_MODEL_ID AND NOT DEFINED FP_ANTI_SPOOFING_KEY)
  message(WARNING "
  -------------------------------------------------------
  --- WARNING: Using demo Fast Pair Model ID and Fast ---
  --- Pair Anti Spoofing Key, it should not be used   ---
  --- for production.                                 ---
  -------------------------------------------------------
  \n"
  )
  set(FP_MODEL_ID "0x2A410B" PARENT_SCOPE)
  set(FP_ANTI_SPOOFING_KEY "Unoh+nycK/ZJ7k3dHsdcNpiP1SfOy0P/Lx5XixyYois=" PARENT_SCOPE)
endif()
