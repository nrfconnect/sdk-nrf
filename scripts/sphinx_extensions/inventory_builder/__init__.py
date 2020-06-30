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

from sphinx.builders import Builder
from sphinx.util.inventory import InventoryFile

import os.path as path


class InventoryBuilder(Builder):
    name = 'inventory'
    epilog = 'Only generates an inventory file.'

    allow_parallel = True

    def init(self):
        pass

    def get_outdated_docs(self):
        return self.env.found_docs

    def get_target_uri(self, docname, typ=None):
        return docname + '.html'

    def prepare_writing(self, docnames):
        pass

    def write_doc(self, docname, doctree):
        pass

    def finish(self):
        InventoryFile.dump(path.join(self.outdir, 'objects.inv'),
                           self.env, self)


def setup(app):
    app.add_builder(InventoryBuilder)

    return {
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
