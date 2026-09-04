"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

from docutils.parsers.rst import directives
from sphinx.util.docutils import SphinxDirective
from vscode_button.common import get_latest_stable_version, render_vscode_button

INSTALL_IN_VSCODE_URL = (
    "vscode://nordic-semiconductor.nrf-connect/install"
    "?sdkType=nRF%20Connect%20SDK"
    "&sdkVersion={version}"
)


class VSCodeSDKInstallDirective(SphinxDirective):
    has_content = False
    required_arguments = 0
    optional_arguments = 0
    option_spec = {
        "version": directives.unchanged,
        "label": directives.unchanged,
    }

    def run(self) -> list:
        version = self.options.get("version") or get_latest_stable_version(self.env.app)
        if not version:
            return []

        uri = INSTALL_IN_VSCODE_URL.format(version=version)
        return render_vscode_button(
            self.env,
            label=self.options.get("label", "Install in VS Code"),
            uri=uri,
            tooltip=(
                "This starts the installation of the nRF Connect SDK and toolchain "
                f"v{version} in VS Code."
            ),
            inline=True,
        )
