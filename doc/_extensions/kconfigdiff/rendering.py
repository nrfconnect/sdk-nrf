#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
Kconfig diff rendering

Docutils rendering for the ``kconfigdiff`` directive.
"""

import difflib
import logging
from dataclasses import dataclass
from pathlib import Path

from docutils import nodes
from sphinx.util.docutils import SphinxDirective

from .kconfig_utils import KCONFIG_SAVE_FILE, KconfigEntryProperties, diff_generator

logger = logging.Logger(__name__)
DIFFER = difflib.Differ()


@dataclass
class ComparisonPair:
    old: KconfigEntryProperties | None
    new: KconfigEntryProperties | None

    def render_property(self, property_, value) -> nodes.field | None:
        if not value:
            return None
        field = nodes.field()
        field += nodes.field_name(text=property_)
        body = nodes.field_body()
        body += nodes.paragraph(text=value)
        field += body
        return field

    def render_list_property(self, property_, values) -> nodes.field | None:
        if not values:
            return None

        field = nodes.field()
        field += nodes.field_name(text=property_)

        body = nodes.field_body()
        values_list = nodes.bullet_list()

        for value in values:
            item = nodes.list_item()
            item += nodes.paragraph(text=value)
            values_list += item

        body += values_list
        field += body

        return field

    def render_diff_property(self, property_, old_val, new_val) -> nodes.field | None:
        if old_val == new_val:
            return self.render_property(property_, new_val)
        field = nodes.field()
        field += nodes.field_name(text=property_)
        body = nodes.field_body()
        body += nodes.paragraph(text=old_val, classes=["kconfigdiff-removed", "kconfigdiff-line"])
        body += nodes.paragraph(text=new_val, classes=["kconfigdiff-added", "kconfigdiff-line"])
        field += body
        return field

    def render_list_diff_property(self, property_, old_values, new_values) -> nodes.field | None:
        if set(old_values) == set(new_values):
            return self.render_list_property(property_, new_values)

        field = nodes.field()
        field += nodes.field_name(text=property_)

        body = nodes.field_body()
        values_list = nodes.bullet_list()

        diff = DIFFER.compare(old_values, new_values)

        for value in diff:
            if value.startswith("?"):
                continue
            item = nodes.list_item()
            if value.startswith("+"):
                item += nodes.paragraph(
                    text=value[1:].strip(), classes=["kconfigdiff-added", "kconfigdiff-line"]
                )
            elif value.startswith("-"):
                item += nodes.paragraph(
                    text=value[1:].strip(), classes=["kconfigdiff-removed", "kconfigdiff-line"]
                )
            else:
                item += nodes.paragraph(text=value.strip())
            values_list += item

        body += values_list
        field += body

        return field

    def render_fields(self, props: KconfigEntryProperties) -> nodes.field_list:
        fields = nodes.field_list()

        fields += self.render_property("Prompt", props.prompt)
        fields += self.render_property("Help", props.help)
        fields += self.render_property("Type", props.type)
        fields += self.render_property("Dependencies", props.dependencies)
        fields += self.render_list_property("Defaults", props.defaults)
        fields += self.render_list_property("Selects", props.selects)
        fields += self.render_list_property("Selected by", props.selected_by)
        fields += self.render_list_property("Implies", props.implies)
        fields += self.render_list_property("Implied by", props.implied_by)
        fields += self.render_list_property("Choices", props.choices)
        fields += self.render_list_property("Ranges", props.ranges)

        return fields

    def render_diff_fileds(self) -> nodes.field_list:
        fields = nodes.field_list()

        fields += self.render_diff_property("Prompt", self.old.prompt, self.new.prompt)
        fields += self.render_diff_property("Help", self.old.help, self.new.help)
        fields += self.render_diff_property("Type", self.old.type, self.new.type)
        fields += self.render_diff_property(
            "Dependencies", self.old.dependencies, self.new.dependencies
        )
        fields += self.render_list_diff_property("Defaults", self.old.defaults, self.new.defaults)
        fields += self.render_list_diff_property("Selects", self.old.selects, self.new.selects)
        fields += self.render_list_diff_property(
            "Selected by", self.old.selected_by, self.new.selected_by
        )
        fields += self.render_list_diff_property("Implies", self.old.implies, self.new.implies)
        fields += self.render_list_diff_property(
            "Implied by", self.old.implied_by, self.new.implied_by
        )
        fields += self.render_list_diff_property("Choices", self.old.choices, self.new.choices)
        fields += self.render_list_diff_property("Ranges", self.old.ranges, self.new.ranges)

        return fields

    def render(self) -> nodes.container | None:
        """Render a single :class:`ComparisonPair` as a docutils node.

        Returns ``None`` for pairs whose properties are identical so the caller can
        skip them.
        """
        if self.new is None:
            # props = extract_properties(pair.old)
            entry = nodes.container(classes=["kconfigdiff-entry", "kconfigdiff-entry-removed"])
            entry += _entry_title(f"{self.old.name}", "removed")  # type: ignore[union-attr]

            entry += self.render_fields(self.old)
            return entry

        if self.old is None:
            # props = extract_properties(pair.new)
            entry = nodes.container(classes=["kconfigdiff-entry", "kconfigdiff-entry-new"])
            entry += _entry_title(f"{self.new.name}", "new")

            entry += self.render_fields(self.new)
            return entry

        if self.old == self.new:
            return None

        entry = nodes.container(classes=["kconfigdiff-entry", "kconfigdiff-entry-changed"])
        entry += _entry_title(f"{self.new.name}", "changed")

        entry += self.render_diff_fileds()

        return entry


def _entry_title(text: str, css_kind: str) -> nodes.paragraph:
    title = nodes.paragraph(classes=["kconfigdiff-entry-title", f"kconfigdiff-title-{css_kind}"])
    title += nodes.strong(text=text)
    return title


class KconfigDiffDirective(SphinxDirective):
    """Render the diff between two pickled Kconfig snapshots.

    Usage::

        .. kconfigdiff:: old.pickle new.pickle
    """

    required_arguments = 0
    optional_arguments = 0
    final_argument_whitespace = False
    has_content = False

    def run(self) -> list[nodes.Node]:
        if not self.env.app.config.kconfigdiff_should_build:
            return []

        if not (versions := self.env.app.config.kconfigdiff_versions):
            return []
        _, prev = versions

        root = nodes.container(classes=["kconfigdiff"])

        def ordering(old, new):
            return max((old, new), key=lambda x: x is not None)

        outdir = Path(self.env.app.outdir)
        for old, new in sorted(
            diff_generator(prev, outdir),
            key=lambda p: ordering(*p).name,
        ):
            node = ComparisonPair(old, new).render()
            if node is not None:
                root += node

        self.env.app.config.html_extra_path.append(outdir / KCONFIG_SAVE_FILE)
        return [nodes.transition(), root]
