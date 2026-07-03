#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

from docutils import nodes
from sphinx.util.docutils import SphinxDirective


class KconfigDiffLegendDirective(SphinxDirective):
    """
    Renders the legend describing how the extension shows comparison.

    Usage::
        .. kconfigdiff-legend
    """

    required_arguments = 0
    optional_arguments = 0

    @staticmethod
    def sample(text: str, type_: str) -> nodes.Node:
        return nodes.paragraph(text=text, classes=["kconfigdiff-line", f"kconfigdiff-{type_}"])

    def run(self) -> list[nodes.Node]:
        res = [
            KconfigDiffLegendDirective.sample("This was added", "added"),
            KconfigDiffLegendDirective.sample("This was removed", "removed"),
            KconfigDiffLegendDirective.sample("This was changed", "changed"),
        ]
        return res
