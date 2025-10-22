# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Import nrf_common.py and override/include additional
functionality required for nRF7120
'''

import os
import sys

ZEPHYR_BASE = os.environ.get('ZEPHYR_BASE')
sys.path.append(f"{ZEPHYR_BASE}/scripts/west_commands/runners")
import nrf_common

nrf_common.UICR_RANGES['nrf71'] = {'Application': (0x00FFD000, 0x00FFDA00)}

class NrfBinaryRunnerNext(nrf_common.NrfBinaryRunner):

    def __init__(self, cfg, family, softreset, pinreset, dev_id, erase=False,
                 erase_mode=None, ext_erase_mode=None, reset=True,
                 tool_opt=None, force=False, recover=False):

        self.family = family # Required to silence git complience checks

        super().__init__(cfg, family, softreset, pinreset, dev_id, erase=False,
                         erase_mode=None, ext_erase_mode=None, reset=reset,
                         tool_opt=None, force=False, recover=False)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--nrf-family',
                            choices=['NRF71'],
                            help='''MCU family; still accepted for
                            compatibility only''')
        # Not using a mutual exclusive group for softreset and pinreset due to
        # the way dump_runner_option_help() works in run_common.py
        parser.add_argument('--softreset', required=False,
                            action='store_true',
                            help='use softreset instead of pinreset')
        parser.add_argument('--pinreset', required=False,
                            action='store_true',
                            help='use pinreset instead of softreset')
        parser.add_argument('--snr', required=False, dest='dev_id',
                            help='obsolete synonym for -i/--dev-id')
        parser.add_argument('--force', required=False,
                            action='store_true',
                            help='Flash even if the result cannot be guaranteed.')
        parser.add_argument('--recover', required=False,
                            action='store_true',
                            help='''erase all user available non-volatile
                            memory and disable read back protection before
                            flashing (erases flash for both cores on nRF53)''')
        parser.add_argument('--erase-mode', required=False,
                            choices=['none', 'ranges', 'all'],
                            help='Select the type of erase operation for the '
                                 'internal non-volatile memory')
        parser.add_argument('--ext-erase-mode', required=False,
                            choices=['none', 'ranges', 'all'],
                            help='Select the type of erase operation for the '
                                 'external non-volatile memory')

        parser.set_defaults(reset=True)


    def ensure_family(self):
        # Ensure self.family is set.

        if self.family is not None:
            return

        if self.build_conf.getboolean('CONFIG_SOC_SERIES_NRF71X'):
            self.family = 'nrf71'
        else:
            raise RuntimeError(f'unknown nRF; update {__file__}')


    def _get_core(self):
        if self.family in ('nrf54h', 'nrf71', 'nrf92'):
            if (self.build_conf.getboolean('CONFIG_SOC_NRF7120_ENGA_CPUAPP') or
                self.build_conf.getboolean('CONFIG_SOC_NRF7120_ENGA_CPUFLPR')):
                return 'Application'
            raise RuntimeError(f'Core not found for family: {self.family}')
        return None
