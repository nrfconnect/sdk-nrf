# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from docutils import io, statemachine
from docutils.utils.error_reporting import ErrorString
from docutils.parsers.rst import directives
from sphinx.util.docutils import SphinxDirective
import os
import yaml


class TableFromRows(SphinxDirective):

    has_content = True
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = True
    option_spec = {
        'header': directives.unchanged,
        'rows': directives.unchanged,
        'sample-yaml-rows': directives.flag,
    }

    def _load_section(self, lines, section):
        found_section = False
        section_lines = []
        for line in lines:
            line = line.strip()
            if line == '.. ' + section:
                found_section = True
                continue
            elif line.startswith('.. ') and found_section:
                break
            if found_section and line != '':
                section_lines.append(line)
        if len(section_lines) == 0:
            raise self.severe('Problems building table; "%s" section not found.' %
                              section)
        return section_lines

    def _column_sizes(self, row):
        cols = row.split('|')
        if len(cols) < 2 or cols[0] != '' or cols[-1] != '':
            raise self.severe('Row does not look like a table: "%s"' % row)
        return [len(el.strip()) for el in cols[1:-1]]

    @staticmethod
    def _adjust_column_sizes(sizes, line):
        columns = []
        for size, column in zip(sizes, line.split('|')[1:-1]):
            column = column.strip()
            columns.append(column + ' ' * (size - len(column)))
        return '|' + '|'.join(columns) + '|'

    def _build_table(self, sizes, header_lines, rows):
        row_sep = '+' + '+'.join(['-' * size for size in sizes]) + '+'
        table = [row_sep]
        # XXX for header_lines to be len==1?
        for line in header_lines:
            table.append(self._adjust_column_sizes(sizes, line))
        table.append('+' + '+'.join(['=' * size for size in sizes]) + '+')
        for row_set in rows:
            for line in row_set:
                table.append(self._adjust_column_sizes(sizes, line))
            table.append(row_sep)
        return table

    def _find_column_sizes(self, header_lines, rows):
        sizes = self._column_sizes(header_lines[0])
        for row in header_lines[1:]:
            row_sizes = self._column_sizes(row)
            if len(sizes) != len(row_sizes):
                raise self.severe('Incompatible number of columns: "%s"' % row)
            sizes = [max(sizes[i], row_sizes[i]) for i, _ in enumerate(sizes)]
        for row_set in rows:
            for row in row_set:
                row_sizes = self._column_sizes(row)
                if len(sizes) != len(row_sizes):
                    raise self.severe('Incompatible number of columns: "%s"' % row)
                sizes = [max(sizes[i], row_sizes[i]) for i, _ in enumerate(sizes)]
        return sizes

    def _load_header_and_rows(self, source_path, header_section, rows_sections):
        try:
            self.state.document.settings.record_dependencies.add(source_path)
            include_file = io.FileInput(source_path=source_path)
        except IOError as error:
            raise self.severe('Problems with "%s" directive path:\n%s.' %
                              (self.name, ErrorString(error)))
        lines = include_file.readlines()
        header_lines = self._load_section(lines, header_section)
        rows = [self._load_section(lines, section) for section in rows_sections]
        column_sizes = self._find_column_sizes(header_lines, rows)
        return self._build_table(column_sizes, header_lines, rows)

    def _rows_from_sample_yaml(self, path):
        if 'NRF_BASE' not in os.environ:
            raise self.severe(":sample-yaml-rows: needs NRF_BASE in the environment")
        if 'NRF_RST_SRC' not in os.environ:
            raise self.severe(":sample-yaml-rows: needs NRF_RST_SRC in the environment")
        rel_path = os.path.relpath(path, os.environ['NRF_RST_SRC'])
        path = os.path.join(os.environ['NRF_BASE'], rel_path, 'sample.yaml')
        try:
            sample_yaml = open(path)
        except FileNotFoundError:
            raise self.severe('"%s" not found' % path)
        data = yaml.load(sample_yaml, Loader=yaml.BaseLoader)
        path_key = '.'.join(rel_path.split(os.path.sep))
        for key in ['tests', path_key, 'platform_allow']:
            if key not in data:
                raise self.severe('Unexpected sample.yaml format detected, '
                                  '"%s" key not found!' % key)
            data = data[key]
        return [r.strip() for r in data.split(' ') if r.strip() != '']

    def run(self):
        _, path = self.env.relfn2path(self.arguments[0])

        header = self.options.get('header', None)
        if header is not None:
            header = header.strip()
        else:
            raise self.severe('Problem with "header" option of "%s" '
                              'directive:\nEmpty content.' % self.name)
        rows = self.options.get('rows', None)
        if rows is not None:
            rows = [r.strip() for r in rows.split(',') if r.strip() != '']
        else:
            rows = []

        if 'sample-yaml-rows' in self.options:
            source = self.state_machine.input_lines.source(
                 self.lineno - self.state_machine.input_offset - 1)
            source_dir = os.path.dirname(os.path.abspath(source))
            sample_yaml_rows = self._rows_from_sample_yaml(source_dir)
        else:
            sample_yaml_rows = []

        if len(rows) == 0 and len(sample_yaml_rows) == 0:
            raise self.severe('No "rows" content or "sample-yaml-rows" '
                              'provided for "%s" ' % self.name)

        # join all rows for uniqueness, using the order given in the :rows:
        # option or inside sample.yaml, and prioritize :rows: order
        all_rows = {}
        if len(rows) > 0:
            all_rows.update(dict.fromkeys(rows))
        if len(sample_yaml_rows) > 0:
            all_rows.update(dict.fromkeys(sample_yaml_rows))
        raw = '\n'.join(self._load_header_and_rows(path, header,
                                                   all_rows.keys()))
        lines = statemachine.string2lines(raw)
        self.state_machine.insert_input(lines, path)
        return []


def setup(app):
    directives.register_directive('table-from-rows', TableFromRows)
