/*
 * Copyright (c) 2026 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

zcode_extender_area :
{
  __code_extender_start = .;
  __code_extender_end = (CONFIG_CODE_EXTENDER_SIZE + __rom_start_address - 0x10);
  BYTE(0xEF);
  FILL(0xAC);
  . = (__code_extender_end > __code_extender_start ? (__code_extender_end - __code_extender_start) : 1);
} > FLASH
