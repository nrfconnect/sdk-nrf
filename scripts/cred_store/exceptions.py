#!/usr/bin/env python3
#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

class NoATClientException(Exception):
    'Raised when the device does not have an AT client'

class ATCommandError(Exception):
    'Raised when an AT command responds with ERROR'
