# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from sphinx.application import Sphinx
from . import options_from_kconfig

__version__ = '0.0.1'


def setup(app: Sphinx):
    options_from_kconfig.setup(app)

    return {
        'version': __version__,
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
