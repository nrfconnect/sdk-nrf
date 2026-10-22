"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

Add glossary definition text as the HTML ``title`` attribute on :term: links.

Click behaviour is unchanged (still navigates to the glossary entry). Hover
shows the browser-native tooltip with the definition text.
"""

from __future__ import annotations

from typing import TYPE_CHECKING

from docutils import nodes
from sphinx.transforms.post_transforms import SphinxPostTransform
from sphinx.util.logging import getLogger

if TYPE_CHECKING:
    from sphinx.application import Sphinx
    from sphinx.util.typing import ExtensionMetadata

logger = getLogger(__name__)

__version__ = "0.1.0"

_TOOLTIP_CACHE_ATTR = "_glossary_term_tooltip_cache"


def _get_tooltip_data(app: Sphinx) -> tuple[dict[str, str], dict[str, str]]:
    """Return (term_key -> definition, labelid -> term_key), computed once per build."""
    env = app.env
    cached = getattr(env, _TOOLTIP_CACHE_ATTR, None)
    if cached is not None:
        return cached

    definitions = _collect_term_definitions(app)
    domain = env.get_domain("std")
    labelid_to_term = {
        labelid: term_key
        for term_key, (_doc, labelid) in domain._terms.items()  # noqa: SLF001
    }
    cached = (definitions, labelid_to_term)
    setattr(env, _TOOLTIP_CACHE_ATTR, cached)
    return cached


def _collect_term_definitions(app: Sphinx) -> dict[str, str]:
    """Map normalized glossary term names to plain-text definitions."""
    definitions: dict[str, str] = {}
    env = app.env
    domain = env.get_domain("std")
    terms: dict[str, tuple[str, str]] = domain._terms  # noqa: SLF001

    for term_key, (docname, _labelid) in terms.items():
        try:
            doctree = env.get_doctree(docname)
        except Exception:
            continue

        for dlist in doctree.findall(nodes.definition_list):
            for item in dlist.children:
                if not isinstance(item, nodes.definition_list_item):
                    continue

                term_node: nodes.term | None = None
                def_node: nodes.definition | None = None
                for child in item.children:
                    if isinstance(child, nodes.term):
                        term_node = child
                    elif isinstance(child, nodes.definition):
                        def_node = child

                if term_node is None or def_node is None:
                    continue

                name = term_node.astext().strip()
                if name.lower() != term_key:
                    continue

                text = def_node.astext().strip()
                if text:
                    definitions[term_key] = text

    return definitions


def _term_fragment(node: nodes.reference) -> str | None:
    """Return the glossary term anchor id (``term-...``) for a reference node."""
    refuri = node.get("refuri")
    if refuri:
        if "#" in refuri:
            fragment = refuri.rsplit("#", 1)[-1]
        else:
            fragment = refuri.lstrip("#")
        if fragment.startswith("term-"):
            return fragment

    refid = node.get("refid")
    if refid and refid.startswith("term-"):
        return refid

    return None


class GlossaryTermTooltipTransform(SphinxPostTransform):
    """Attach glossary definitions to resolved :term: references."""

    default_priority = 399

    def run(self, **kwargs) -> None:
        definitions, labelid_to_term = _get_tooltip_data(self.app)
        if not definitions:
            return

        for node in self.document.findall(nodes.reference):
            fragment = _term_fragment(node)
            if fragment is None:
                continue

            term_key = labelid_to_term.get(fragment)
            if term_key is None:
                continue

            definition = definitions.get(term_key)
            if not definition or node.get("reftitle"):
                continue

            # HTML5Translator maps ``reftitle`` to the anchor ``title`` attribute.
            node["reftitle"] = definition


def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_post_transform(GlossaryTermTooltipTransform)
    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
