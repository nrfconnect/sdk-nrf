# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from . import table_from_rows

__version__ = '0.0.1'


def setup(app):
    app.add_config_value("table_from_rows_base_dir", None, "env")

    table_from_rows.setup(app)

    return {
        'version': __version__,
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
