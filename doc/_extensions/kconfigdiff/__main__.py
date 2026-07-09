#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
Kconfig snapshot generator

Run with ``python -m kconfigdiff`` to load the Kconfig tree and write a pickled
snapshot that the ``kconfigdiff`` directive can diff.
"""

import os

from . import pickler
from .kconfig_utils import NRF_BASE, ZEPHYR_BASE, kconfig_load


def main() -> None:
    os.environ['NCS_MEMFAULT_FIRMWARE_SDK_KCONFIG'] = str(
        NRF_BASE / 'modules/memfault-firmware-sdk/Kconfig'
    )
    os.environ['ZEPHYR_NRF_KCONFIG'] = str(NRF_BASE / 'Kconfig.nrf')
    os.environ['SYSBUILD_NRF_KCONFIG'] = str(NRF_BASE / 'sysbuild/Kconfig.sysbuild')

    # Unset toolchain variant to match docbuild.yml environment
    os.environ['ZEPHYR_TOOLCHAIN_VARIANT'] = ""

    kconfig, sysbuild_kconfig, _ = kconfig_load([ZEPHYR_BASE, NRF_BASE])

    pickler.save_kconfig("kconfig.zip", kconfig, sysbuild_kconfig)


if __name__ == "__main__":
    main()
