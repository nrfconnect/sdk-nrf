# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
# This is based on the dummy builder from Sphinx, which is
# BSD licensed.
#
# This package can be added as an extension, and if run as a
# builder (sphinx -b inventory), it will not generated any
# output files apart from an inventory file (objects.inv).
#
# It can be run as a 1st step when projects contain multi-
# directional links between them.

from pathlib import Path
from typing import Iterator, Set

from docutils import nodes
from sphinx.builders import Builder
from sphinx.util.inventory import InventoryFile


class InventoryBuilder(Builder):
    name = 'inventory'
    epilog = 'Only generates an inventory file.'

    allow_parallel = True

    def init(self) -> None:
        pass

    def get_outdated_docs(self) -> Iterator[str]:
        for doc_name in self.env.found_docs:
            # check if doc is new
            if doc_name not in self.env.all_docs:
                yield doc_name
                continue

            # check if source has been modified
            src = Path(self.env.doc2path(doc_name))
            if src.stat().st_mtime > self.env.all_docs[doc_name]:
                yield doc_name
                continue

    def get_target_uri(self, docname: str, typ: str = None) -> str: #pylint: disable=no-self-use
        return docname + '.html'

    def prepare_writing(self, docnames: Set[str]) -> None:
        pass

    def write_doc(self, docname: str, doctree: nodes.document) -> None:
        pass

    def finish(self) -> None:
        InventoryFile.dump(Path(self.outdir) / 'objects.inv', self.env, self)


def setup(app):
    app.add_builder(InventoryBuilder)

    return {
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
