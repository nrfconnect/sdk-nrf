# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import os
import os.path
import sys

from docutils import statemachine
from docutils.parsers.rst import directives
from sphinx.application import Sphinx
from sphinx.util.docutils import SphinxDirective


__version__ = '0.0.1'


class OptionsFromKconfig(SphinxDirective):

    has_content = True
    required_arguments = 0
    optional_arguments = 1
    final_argument_whitespace = True
    option_spec = {
        'prefix': directives.unchanged,
        'suffix': directives.unchanged,
        'show-type': directives.flag,
        'only-visible': directives.flag,
    }

    @staticmethod
    def _monkey_patch_kconfiglib(kconfiglib):
        '''
        Disable kconfiglib from building a config tree by importing other
        Kconfig elements, from Zephyr, etc. This way only the symbols defined
        inside the parsed Kconfig file itself will be available to be used by
        this extension.
        '''
        kconfiglib._SOURCE_TOKENS = kconfiglib._REL_SOURCE_TOKENS
        kconfiglib.Kconfig._parse_error = lambda self_, msg: None

    def _get_kconfig_path(self, path):
        rel_path = os.path.relpath(path, self.env.srcdir)
        return os.path.join(
            self.config.options_from_kconfig_base_dir, rel_path, 'Kconfig'
        )

    def run(self):
        if len(self.arguments) > 0:
            _, path = self.env.relfn2path(self.arguments[0])
        else:
            source = self.state_machine.input_lines.source(
                self.lineno - self.state_machine.input_offset - 1)
            source_dir = os.path.dirname(os.path.abspath(source))
            path = self._get_kconfig_path(source_dir)

        sys.path.append(
            os.path.join(
                self.config.options_from_kconfig_zephyr_dir,
                'scripts',
                'kconfig'
            )
        )
        os.environ["ZEPHYR_BASE"] = str(self.config.options_from_kconfig_zephyr_dir)
        import kconfiglib
        self._monkey_patch_kconfiglib(kconfiglib)

        # kconfiglib wants this env var defined
        os.environ['srctree'] = os.path.dirname(os.path.abspath(__file__))
        kconfig = kconfiglib.Kconfig(filename=path, warn=False)

        prefix = self.options.get('prefix', None)
        suffix = self.options.get('suffix', None)

        lines = []
        for sym in kconfig.unique_defined_syms:
            lines.append(f'.. option:: CONFIG_{sym.name}\n')
            if 'only-visible' in self.options:
                visible = kconfiglib.TRI_TO_STR[sym.visibility]
                if visible == 'n':
                    continue
            text = ''
            if 'show-type' in self.options:
                typ_ = kconfiglib.TYPE_TO_STR[sym.type]
                text += '``(' + typ_ + ')`` '
            if prefix is not None:
                if (prefix.startswith('"') and prefix.endswith('"')) or \
                   (prefix.startswith("'") and prefix.endswith("'")):
                    prefix = prefix[1:-1]
                text += prefix
            try:
                prompt_ = f'{sym.nodes[0].prompt[0]}'
            except Exception:
                prompt_ = ''
            if prefix is not None:
                text += prompt_[:1].lower() + prompt_[1:]
            else:
                text += prompt_
            if suffix is not None:
                if (suffix.startswith('"') and suffix.endswith('"')) or \
                   (suffix.startswith("'") and suffix.endswith("'")):
                    suffix = suffix[1:-1]
                text += suffix
            lines.append(f'{text}\n')
            try:
                help_ = sym.nodes[0].help
                lines.append(f'{help_}\n')
            except Exception:
                pass

        lines = statemachine.string2lines('\n'.join(lines))
        self.state_machine.insert_input(lines, path)
        return []


def setup(app: Sphinx):
    app.add_config_value("options_from_kconfig_base_dir", None, "env")
    app.add_config_value("options_from_kconfig_zephyr_dir", None, "env")

    directives.register_directive('options-from-kconfig', OptionsFromKconfig)

    return {
        'version': __version__,
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
