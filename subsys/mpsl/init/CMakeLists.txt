#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(mpsl_init.c)
# mpsl_init.c contains ISR implementations. Those ISRs calls extern functions.
# clang is unable to see if those functions contains floating point instructions because they are
# linked at compile time. Therefore clang emits
# > warning: interrupt service routine with vfp enabled may clobber the interruptee's vfp state;
# > consider using the `interrupt_save_fp` attribute to prevent this behavior
# > [-Warm-interrupt-vfp-clobber]
# Extern calls from mpsl ISRs are not using floating point instructions, so disable the warning.
set_source_files_properties(mpsl_init.c PROPERTIES COMPILE_FLAGS -Wno-arm-interrupt-vfp-clobber)
