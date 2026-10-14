"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

Sphinx extension that provides the ``nrfutil-command`` directive.

Sphinx is imported only in setup(), so the walker and the snapshot modules can be used without it.
"""


def setup(app):
    from nrfutil_command.directive import setup as _setup

    return _setup(app)
