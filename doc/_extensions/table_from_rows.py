# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path
from docutils import io, statemachine
from docutils.utils.error_reporting import ErrorString
from docutils.parsers.rst import directives
from sphinx.util.docutils import SphinxDirective
from typing import Dict, Set
import os
import yaml
import re


__version__ = '0.0.2'


class TableFromRows(SphinxDirective):

    has_content = True
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = True
    option_spec = {
        'header': directives.unchanged,
        'rows': directives.unchanged,
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

    def _load_header_and_rows(
        self,
        source_path,
        header_section,
        rows_sections,
        transform_rows_func=None,
        shields=None,
    ):
        try:
            self.state.document.settings.record_dependencies.add(source_path)
            include_file = io.FileInput(source_path=source_path)
        except IOError as error:
            raise self.severe('Problems with "%s" directive path:\n%s.' %
                              (self.name, ErrorString(error)))
        lines = include_file.readlines()
        header_lines = self._load_section(lines, header_section)
        rows = [self._load_section(lines, section) for section in rows_sections]
        if shields:
            header_lines[0] += ' Shields |'
            for i, target in enumerate(rows_sections):
                if target in shields:
                    rows[i][0] += f'``{"`` ``".join(shields[target])}``'
                rows[i][0] += ' |'

        if transform_rows_func:
            transform_rows_func(rows)
        column_sizes = self._find_column_sizes(header_lines, rows)
        return self._build_table(column_sizes, header_lines, rows)

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
            raise self.severe('No "rows" content provided for "%s" '
                              % self.name)

        # join all rows for uniqueness, using the order given in
        # the :rows: option.
        all_rows = {}
        all_rows.update(dict.fromkeys(rows))
        raw = '\n'.join(self._load_header_and_rows(path, header,
                                                   all_rows.keys()))
        lines = statemachine.string2lines(raw)
        self.state_machine.insert_input(lines, path)
        return []

class TableFromSampleYaml(TableFromRows):

    option_spec = {}
    required_arguments = 0

    @staticmethod
    def _merge_rows(rows):
        """Merge rows where all columns except the build target are equal."""

        seen = {}
        to_delete = []
        for row in rows:
            columns = row[0].split('|')[1:-1]
            common_info = '|'.join(columns[:3]) + '|' + '|'.join(columns[4:])
            build_target = columns[3]
            if common_info in seen:
                extended_row = f'| | | | {build_target} |' + ' |' * len(columns[4:])
                seen[common_info].append(extended_row)
                to_delete.append(row)
            else:
                seen[common_info] = row

        for row in to_delete:
            rows.remove(row)

    @staticmethod
    def _find_shields(shields: Dict[str, Set[str]], sample_data: dict):
        """Associate all integration platforms for a sample with any shield used.
        """

        if 'extra_args' not in sample_data:
            return

        shield_args = re.findall(r'SHIELD=(\S*)', sample_data['extra_args'])
        if not shield_args:
            return

        for platform in sample_data['integration_platforms']:
            if platform in shields:
                shields[platform].update(shield_args)
            else:
                shields[platform] = set(shield_args)

    def _rows_from_sample_yaml(self, path):
        """Search for a sample.yaml file and return a union of all relevant boards.

        Boards are retrieved from the integration_platforms sections in the file.
        If the file is not found and the document folder is named "doc", the
        parent folder is searched. Should a sample.yaml still not be found, all
        subfolders are searched and every found sample.yaml is used.

        Shields are retrieved from the extra_args sections in the file for every
        argument prefixed with "SHIELD=".

        nrf51 and qemu boards are ignored."""

        # Path from rst-file in build folder to source build directory.
        rel_path = os.path.relpath(path, self.env.srcdir)

        # In cases where ``path`` for some reason is not pointing to the
        # build folder, remove all instances of '../' from the path.
        rel_path = rel_path.lstrip('..' + os.path.sep)

        # Path to the origin of the rst-file outside the build folder
        dir_path = Path(self.config.table_from_rows_base_dir) / rel_path

        sample_yamls = []
        if (dir_path / 'sample.yaml').exists():
            sample_yamls.append(dir_path / 'sample.yaml')
        elif rel_path.endswith('doc'):
            parent_path = dir_path.parent / 'sample.yaml'
            if parent_path.exists():
                sample_yamls.append(parent_path)
        else:
            sample_yamls += list(dir_path.glob('**/sample.yaml'))

        if not sample_yamls:
            raise self.severe(f'"{path}" not found')

        boards = set()
        shields = {}
        for sample_yaml_path in sample_yamls:
            with open(sample_yaml_path) as sample_yaml:
                data = yaml.safe_load(sample_yaml)

            if 'common' in data and 'integration_platforms' in data['common']:
                boards.update(data['common']['integration_platforms'])
                self._find_shields(shields, data['common'])
            for test in data['tests'].values():
                if 'integration_platforms' in test:
                    boards.update(test['integration_platforms'])
                    self._find_shields(shields, test)

        boards = list(filter(
            lambda b: not b.startswith('qemu') and not b.startswith('nrf51'),
            boards,
        ))
        boards.sort(reverse=True)

        if not boards:
            raise self.severe(f'{path} has no relevant integration_platforms')

        return boards, shields

    def run(self):
        reference_file = self.config.table_from_sample_yaml_board_reference
        _, path = self.env.relfn2path(reference_file)

        source = self.state_machine.input_lines.source(
                self.lineno - self.state_machine.input_offset - 1)
        source_dir = os.path.dirname(os.path.abspath(source))
        sample_yaml_rows, shields = self._rows_from_sample_yaml(source_dir)

        raw = '\n'.join(self._load_header_and_rows(
            path,
            'heading',
            sample_yaml_rows,
            transform_rows_func=self._merge_rows,
            shields=shields,
        ))
        lines = statemachine.string2lines(raw)
        self.state_machine.insert_input(lines, path)
        return []


def setup(app):
    app.add_config_value('table_from_rows_base_dir', None, 'env')
    app.add_config_value('table_from_sample_yaml_board_reference', None, 'env')

    directives.register_directive('table-from-rows', TableFromRows)
    directives.register_directive('table-from-sample-yaml', TableFromSampleYaml)

    return {
        'version': __version__,
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
