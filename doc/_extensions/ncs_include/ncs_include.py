# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

# The code for this directive is based on the Include directive from
# docutils, which is public-domain, stripped of lots of its original
# options, and with added support for NCS documentation sets and
# indentation # of the included rST.

import os
import os.path

from docutils import io, statemachine
from docutils.utils.error_reporting import SafeString, ErrorString
from docutils.parsers.rst import directives
from sphinx.util.docutils import SphinxDirective


class NcsInclude(SphinxDirective):
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = True
    option_spec = {
        'docset': directives.unchanged,
        'indent': int,
        'dedent': int,
        'auto-dedent': directives.flag,
        'tab-width': int,
        'start-line': int,
        'end-line': int,
        'start-after': directives.unchanged_required,
        'end-before': directives.unchanged_required,
        'start-at': directives.unchanged_required,
        'end-at': directives.unchanged_required,
    }

    def run(self):
        if not isinstance(self.config.ncs_include_mapping, dict):
            raise self.severe('Invalid "ncs_include_mapping" config')

        docset = self.options.get('docset', None)
        if docset is not None and \
                docset not in self.config.ncs_include_mapping:
            raise self.severe('The supplied "docset" was not found: "%s"' %
                              docset)

        indent = self.options.get('indent', 0)
        dedent = self.options.get('dedent', 0)
        auto_dedent = 'auto-dedent' in self.options

        if indent and (dedent or auto_dedent):
            raise self.severe('Both "indent" and one of the "dedent" options '
                              'are set, please use only one')

        if dedent and auto_dedent:
            raise self.severe('Choose one of "dedent" and "auto-dedent" only')

        if not self.state.document.settings.file_insertion_enabled:
            raise self.warning('"%s" directive disabled.' % self.name)

        if docset is None:
            # without a docset fallback to Sphinx style include
            _, path = self.env.relfn2path(self.arguments[0])
        else:
            path = os.path.join(self.config.ncs_include_mapping[docset],
                                self.arguments[0])

        encoding = self.options.get(
            'encoding', self.state.document.settings.input_encoding)
        e_handler = self.state.document.settings.input_encoding_error_handler
        tab_width = self.options.get(
            'tab-width', self.state.document.settings.tab_width)
        try:
            self.state.document.settings.record_dependencies.add(path)
            include_file = io.FileInput(source_path=path,
                                        encoding=encoding,
                                        error_handler=e_handler)
        except UnicodeEncodeError:
            raise self.severe('Problems with "%s" directive path:\n'
                              'Cannot encode input file path "%s" '
                              '(wrong locale?).' %
                              (self.name, SafeString(path)))
        except IOError as error:
            raise self.severe('Problems with "%s" directive path:\n%s.' %
                              (self.name, ErrorString(error)))

        # Get to-be-included content
        startline = self.options.get('start-line', None)
        endline = self.options.get('end-line', None)
        try:
            if startline or (endline is not None):
                lines = include_file.readlines()
                rawtext = ''.join(lines[startline:endline])
            else:
                rawtext = include_file.read()
        except UnicodeError as error:
            raise self.severe(u'Problem with "%s" directive:\n%s' %
                              (self.name, ErrorString(error)))
        # start-after/end-before: no restrictions on newlines in match-text,
        # and no restrictions on matching inside lines vs. line boundaries
        start_at_text = self.options.get('start-at', None)
        after_text = self.options.get('start-after', start_at_text)
        if after_text:
            # skip content in rawtext before *and incl.* a matching text
            after_index = rawtext.find(after_text)
            if after_index < 0:
                raise self.severe('Problem with "start-after" option of "%s" '
                                  'directive:\nText not found.' % self.name)
            if start_at_text:
                rawtext = rawtext[after_index:]
            else:
                rawtext = rawtext[after_index + len(after_text):]
        end_at_text = self.options.get('end-at', None)
        before_text = self.options.get('end-before', end_at_text)
        if before_text:
            # skip content in rawtext after *and incl.* a matching text
            before_index = rawtext.find(before_text)
            if before_index < 0:
                raise self.severe('Problem with "end-before" option of "%s" '
                                  'directive:\nText not found.' % self.name)
            if end_at_text:
                rawtext = rawtext[:before_index + len(end_at_text)]
            else:
                rawtext = rawtext[:before_index]

        include_lines = statemachine.string2lines(rawtext, tab_width,
                                                  convert_whitespace=True)

        def is_blank(line):
            # XXX might need to check for a stripped line
            return len(line) == 0

        if auto_dedent:
            min_spaces = None
            for i, line in enumerate(include_lines):
                if is_blank(line):
                    continue
                spaces = len(line) - len(line.lstrip(' '))
                if min_spaces is None:
                    min_spaces = spaces
                else:
                    min_spaces = min(spaces, min_spaces)
                print(min_spaces, line)
                # it can't get smaller, so leave early
                if min_spaces == 0:
                    break
            amount = min_spaces

            for i, line in enumerate(include_lines):
                if is_blank(line):
                    continue
                include_lines[i] = line[amount:]

        if dedent:
            for i, line in enumerate(include_lines):
                if is_blank(line):
                    continue
                spaces = len(line) - len(line.lstrip())
                amount = min(spaces, dedent)
                include_lines[i] = line[amount:]

        if indent:
            spaces = ' ' * indent
            for i, line in enumerate(include_lines):
                if is_blank(line):
                    continue
                include_lines[i] = spaces + line

        self.state_machine.insert_input(include_lines, path)
        return []


def setup(app):
    app.add_config_value('ncs_include_mapping', {}, True)
    directives.register_directive('ncs-include', NcsInclude)
