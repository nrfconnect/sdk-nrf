#!/usr/bin/env python3
#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from sphinx.transforms.post_transforms import SphinxPostTransform
from sphinx import addnodes


__version__ = '0.1.0'


class DoxygenIdentifierReferenceResolver(SphinxPostTransform):

    # must run before sphinx ReferenceResolver
    default_priority = 5
    # only resolve identifier xrefs for these domains
    domains = set(['c', 'cpp'])

    def run(self, **kwargs):
        for node in self.document.traverse(addnodes.pending_xref):
            if node['reftype'] != 'identifier':
                continue

            if 'refdomain' not in node or node['refdomain'] not in self.domains:
                continue

            # skip when information required by intersphinx is available already
            if 'refdoc' in node:
                continue

            nodecopy = node.deepcopy()
            contnode = node[0].deepcopy()

            # 'refdoc' is used by intersphinx to correctly adjust relative
            # paths when resolving external references
            nodecopy['refdoc'] = self.env.docname

            newnode = self.app.emit_firstresult('missing-reference', self.env,
                                                nodecopy, contnode)
            if newnode is not None:
                node.replace_self(newnode)


def setup(app):
    app.add_post_transform(DoxygenIdentifierReferenceResolver)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
