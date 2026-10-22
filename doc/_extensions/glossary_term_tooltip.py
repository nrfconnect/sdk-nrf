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
from sphinx.domains.std import StandardDomain

if TYPE_CHECKING:
    from sphinx.addnodes import pending_xref
    from sphinx.application import Sphinx
    from sphinx.builders import Builder
    from sphinx.environment import BuildEnvironment
    from sphinx.util.typing import ExtensionMetadata

__version__ = "0.1.0"

_DEFINITIONS_CACHE_ATTR = "_glossary_term_tooltip_definitions"


def _collect_term_definitions(env: BuildEnvironment) -> dict[str, str]:
    """Map normalized glossary term names to plain-text definitions."""
    domain = env.get_domain("std")
    docnames: set[str] = set()
    for (objtype, _name), (docname, _labelid) in domain.objects.items():
        if objtype == "term":
            docnames.add(docname)

    definitions: dict[str, str] = {}
    for docname in docnames:
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
                text = def_node.astext().strip()
                if name and text:
                    definitions[name.lower()] = text

    return definitions


def _get_term_definitions(env: BuildEnvironment) -> dict[str, str]:
    cached = getattr(env, _DEFINITIONS_CACHE_ATTR, None)
    if cached is not None:
        return cached

    definitions = _collect_term_definitions(env)
    setattr(env, _DEFINITIONS_CACHE_ATTR, definitions)
    return definitions


class GlossaryStandardDomain(StandardDomain):
    """Standard domain that adds glossary definitions to :term: xrefs."""

    def resolve_xref(
        self,
        env: BuildEnvironment,
        fromdocname: str,
        builder: Builder,
        typ: str,
        target: str,
        node: pending_xref,
        contnode: nodes.Element,
    ) -> nodes.reference | None:
        result = super().resolve_xref(env, fromdocname, builder, typ, target, node, contnode)
        if typ != "term" or result is None or result.get("reftitle"):
            return result

        definitions = _get_term_definitions(env)
        definition = definitions.get(target.lower())
        if definition:
            # HTML5Translator maps ``reftitle`` to the anchor ``title`` attribute.
            result["reftitle"] = definition

        return result


def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_domain(GlossaryStandardDomain, override=True)
    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
