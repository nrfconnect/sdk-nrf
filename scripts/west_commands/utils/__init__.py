# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import json
import sys
import traceback


def install_json_excepthook():
    def excepthook(type, value, tb):
        output = {
            "errors": [
                {
                    "type": type.__name__,
                    "message": str(value),
                    "traceback": "".join(traceback.format_tb(tb))
                }
            ]
        }

        print(json.dumps(output, indent=2), file=sys.stderr)
        sys.exit(1)

    sys.excepthook = excepthook
