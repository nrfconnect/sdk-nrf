"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

import io
import sys
from pathlib import Path

import pytest

pytest.importorskip("sphinx")

from sphinx.application import Sphinx  # noqa: E402
from sphinx.util.console import nocolor  # noqa: E402

EXTENSIONS_DIR = Path(__file__).resolve().parents[2] / "_extensions"
sys.path.insert(0, str(EXTENSIONS_DIR))

INDEX = """\
Test
####

.. parsed-literal::
   :class: highlight

   west init --mr |release|

.. parsed-literal::

   west update *project*

.. code-block:: console

   west build
"""


def build(tmp_path):
    src = tmp_path / "src"
    src.mkdir()
    (src / "conf.py").write_text(
        f"import sys\n"
        f"sys.path.insert(0, {str(EXTENSIONS_DIR)!r})\n"
        f"extensions = ['parsed_literal_block']\n"
        f"rst_epilog = '.. |release| replace:: v3.4.1\\n'\n",
        encoding="utf-8",
    )
    (src / "index.rst").write_text(INDEX, encoding="utf-8")
    nocolor()
    app = Sphinx(
        str(src), str(src), str(tmp_path / "out"), str(tmp_path / "doctrees"), "html",
        status=None, warning=io.StringIO(),
    )
    app.build()
    return (tmp_path / "out" / "index.html").read_text(encoding="utf-8")


def test_parsed_literal_blocks_get_the_code_block_wrapper(tmp_path):
    html = build(tmp_path)

    assert "literal-block" not in html
    assert (
        '<div class="highlight-parsed-literal notranslate"><div class="highlight">'
        '<pre class="highlight">west init --mr v3.4.1</pre></div>'
    ) in html
    assert (
        '<div class="highlight-parsed-literal notranslate"><div class="highlight">'
        "<pre>west update <em>project</em></pre></div>"
    ) in html
    assert html.count('<div class="highlight">') == 3
