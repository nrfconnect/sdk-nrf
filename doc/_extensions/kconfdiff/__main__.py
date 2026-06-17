"""
Kconfig snapshot generator

Run with ``python -m kconfdiff`` to load the Kconfig tree and write a pickled
snapshot that the ``kconfdiff`` directive can diff.
"""

import os

from . import pickler
from .kconf_utils import NRF_BASE, ZEPHYR_BASE, kconfig_load


def main() -> None:
    os.environ['NCS_MEMFAULT_FIRMWARE_SDK_KCONFIG'] = str(
        NRF_BASE / 'modules/memfault-firmware-sdk/Kconfig'
    )
    os.environ['ZEPHYR_NRF_KCONFIG'] = str(NRF_BASE / 'Kconfig.nrf')
    os.environ['SYSBUILD_NRF_KCONFIG'] = str(NRF_BASE / 'sysbuild/Kconfig.sysbuild')

    kconfig, sysbuild_kconfig, _ = kconfig_load([ZEPHYR_BASE, NRF_BASE])

    pickler.save_kconfig("kconf.zip", kconfig, sysbuild_kconfig)


if __name__ == "__main__":
    main()
