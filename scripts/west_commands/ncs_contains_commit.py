# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import json
import re
import subprocess
from argparse import ArgumentParser, Namespace
from typing import Any

import tabulate
import yaml
from west.commands import CommandError, WestCommand


class NcsContainsCommit(WestCommand):
    def __init__(self) -> None:
        super().__init__(
            name='ncs-contains-commit',
            help='Show which NCS releases contain given commit(s)',
            description='Show which NCS releases contain given commit(s)',
        )

    def do_add_parser(self, parser_adder: Any) -> ArgumentParser:
        parser = parser_adder.add_parser(self.name, help=self.help, description=self.description)

        parser.add_argument(
            '--subject',
            help='subject of commit to search for',
            type=str,
            action='append',
            dest='subjects',
            required=True,
        )

        parser.add_argument(
            '--from-ncs-tag',
            help='Oldest release to check (vx.x)',
            type=str,
            default='v3.2',
        )

        parser.add_argument(
            '--from-zephyr-tag',
            help='Oldest release to check (vx.x)',
            type=str,
            default='v4.2',
        )

        parser.add_argument(
            '--timestamp',
            help='Print NCS branch availability timestamps after the NCS presence table',
            action='store_true',
        )

        parser.add_argument(
            '--ncs',
            help='Print the NCS presence table',
            action='store_true',
        )

        parser.add_argument(
            '--zephyr',
            help='Print the upstream Zephyr presence table',
            action='store_true',
        )

        parser.add_argument(
            '--json',
            help='Print results as JSON instead of tables',
            action='store_true',
        )

        return parser

    def do_run(self, args: Namespace, _: list[str]) -> None:
        self.args = args
        self.nrf_abspath = self.manifest.repo_abspath
        self.nrf_upstream_url = 'https://github.com/nrfconnect/sdk-nrf'
        zephyr_project = self.manifest.get_projects(['zephyr'], only_cloned=True)[0]
        self.zephyr_abspath = zephyr_project.abspath
        self.zephyr_downstream_url = zephyr_project.url
        self.zephyr_upstream_url = 'https://github.com/zephyrproject-rtos/zephyr'

        self._validate_args_()

        subjects = [subject.rstrip() for subject in args.subjects]

        show_ncs, show_zephyr = self._table_selection_()
        quiet = self.args.json

        ncs_data: list[list[str]] | None = None
        ncs_timestamp_data: list[list[str]] | None = None
        ncs_timestamps: list[list[int | None]] | None = None
        zephyr_data: list[list[str]] | None = None

        if show_ncs:
            if not quiet:
                self.inf('')
                self.inf('Fetching NCS releases')
            ncs_nrf_releases = self._get_releases_(self.nrf_upstream_url, self.args.from_ncs_tag)
            self._fetch_releases_(self.nrf_abspath, self.nrf_upstream_url, ncs_nrf_releases)
            ncs_releases = self._ncs_shas_to_zephyr_shas_(ncs_nrf_releases)
            self._fetch_releases_(self.zephyr_abspath, self.zephyr_upstream_url, ncs_releases)
            ncs_data = self._check_releases_(ncs_releases, subjects, 'Checking NCS releases', quiet)

            if self.args.timestamp:
                if not quiet:
                    self.inf('')
                    self.inf('Checking NCS commit timestamps')
                ncs_timestamp_data, ncs_timestamps = self._resolve_ncs_timestamps_(
                    ncs_nrf_releases,
                    ncs_releases,
                    ncs_data,
                    subjects,
                )

        if show_zephyr:
            if not quiet:
                self.inf('')
                self.inf('Fetching zephyr releases')
            zephyr_releases = self._get_releases_(
                self.zephyr_upstream_url, self.args.from_zephyr_tag
            )
            self._fetch_releases_(self.zephyr_abspath, self.zephyr_upstream_url, zephyr_releases)
            zephyr_data = self._check_releases_(
                zephyr_releases, subjects, 'Checking zephyr releases', quiet
            )

        if self.args.json:
            report = self._build_json_report_(
                subjects,
                show_ncs,
                show_zephyr,
                ncs_data,
                zephyr_data,
                ncs_timestamp_data,
                ncs_timestamps,
            )
            self.inf(json.dumps(report, indent=2))
            return

        self.inf('')
        self.inf('Commit overview')
        for i, subject in enumerate(subjects):
            self.inf(f'  {i}: {subject}')

        if show_ncs:
            ncs_headers = ['NCS tag', 'Zephyr rev'] + list(range(len(subjects)))
            self.inf('')
            self.inf(tabulate.tabulate(ncs_data, headers=ncs_headers))

            if self.args.timestamp:
                self.inf('')
                self.inf('NCS commit timestamps')
                self.inf('')
                self.inf(
                    self._format_ncs_timestamp_table_(
                        ncs_timestamp_data,
                        ncs_timestamps,
                        subjects,
                    )
                )

        if show_zephyr:
            zephyr_headers = ['Zephyr tag', 'Zephyr rev'] + list(range(len(subjects)))
            self.inf('')
            self.inf(tabulate.tabulate(zephyr_data, headers=zephyr_headers))

    def _table_selection_(self) -> tuple[bool, bool]:
        if self.args.ncs or self.args.zephyr:
            return self.args.ncs, self.args.zephyr

        return True, True

    def _build_json_report_(
        self,
        subjects: list[str],
        show_ncs: bool,
        show_zephyr: bool,
        ncs_data: list[list[str]] | None,
        zephyr_data: list[list[str]] | None,
        ncs_timestamp_data: list[list[str]] | None,
        ncs_timestamps: list[list[int | None]] | None,
    ) -> dict[str, Any]:
        report: dict[str, Any] = {'subjects': subjects}

        if show_ncs and ncs_data is not None:
            report['ncs'] = self._presence_rows_to_json_(ncs_data, subjects)

        if self.args.timestamp and ncs_timestamp_data is not None and ncs_timestamps is not None:
            report['ncs_timestamps'] = self._timestamp_rows_to_json_(
                ncs_timestamp_data,
                ncs_timestamps,
            )

        if show_zephyr and zephyr_data is not None:
            report['zephyr'] = self._presence_rows_to_json_(zephyr_data, subjects)

        return report

    def _presence_rows_to_json_(
        self, rows: list[list[str]], subjects: list[str]
    ) -> list[dict[str, Any]]:
        result = []

        for row in rows:
            result.append(
                {
                    'tag': row[0],
                    'zephyr_rev': row[1],
                    'commits': row[2 : 2 + len(subjects)],
                }
            )

        return result

    def _timestamp_rows_to_json_(
        self,
        rows: list[list[str]],
        timestamps: list[list[int | None]],
    ) -> list[dict[str, Any]]:
        result = []

        for row, timestamp_row in zip(rows, timestamps, strict=True):
            result.append(
                {
                    'tag': row[0],
                    'zephyr_rev': row[1],
                    'timestamps': timestamp_row,
                }
            )

        return result

    def _validate_args_(self):
        version_expr = re.compile(r'^v[0-9]+\.[0-9]+$')

        show_ncs, _ = self._table_selection_()
        if self.args.timestamp and not show_ncs:
            raise CommandError('--timestamp requires NCS output (use --ncs)')

        if version_expr.fullmatch(self.args.from_ncs_tag) is None:
            raise CommandError(
                f'--from-ncs-tag {self.args.from_ncs_tag} must follow: v[major].[minor]'
            )

        if version_expr.fullmatch(self.args.from_zephyr_tag) is None:
            raise CommandError(
                f'--from-zephyr-tag {self.args.from_zephyr_tag} must follow: v[major].[minor]'
            )

    def _run_cmd_(self, cwd: str, cmd: str, *cmd_args: str) -> str:
        args = [cmd] + list(cmd_args)
        result = subprocess.run(args, capture_output=True, cwd=cwd)

        if result.returncode != 0:
            raise CommandError(' '.join(args))

        return result.stdout.decode()

    def _filter_tag_(self, tag: str, since_tag: str) -> bool:
        if tag == 'main':
            return True

        return tag >= since_tag

    def _fetch_releases_(
        self,
        cwd: str,
        upstream_url: str,
        releases: list[tuple[str, str]],
    ):
        shas = {sha for sha, _ in releases}

        if not shas:
            return

        self._run_cmd_(
            cwd,
            'git',
            'fetch',
            upstream_url,
            *shas,
        )

    def _check_releases_(
        self,
        releases: list[tuple[str, str]],
        subjects: list[str],
        checking_label: str,
        quiet: bool,
    ) -> list[list[str]]:
        if not quiet:
            self.inf('')
            self.inf(checking_label)

        data = []

        for sha, tag in releases:
            if not quiet:
                self.inf(f'  {tag} ...')
            present = [tag, sha[:8]]
            for subject in subjects:
                present.append('Yes' if self._subject_is_in_zephyr_tree_(sha, subject) else 'No')
            data.append(present)

        return data

    def _resolve_zephyr_sha_(self, rev: str) -> str:
        self._run_cmd_(self.zephyr_abspath, 'git', 'fetch', self.zephyr_downstream_url, rev)
        return self._run_cmd_(self.zephyr_abspath, 'git', 'rev-parse', 'FETCH_HEAD').strip()

    def _get_releases_(self, url: str, from_tag: str) -> list[tuple[str, str]]:
        content = self._run_cmd_(self.zephyr_abspath, 'git', 'ls-remote', url)

        releases_expr = re.compile(
            r'^([a-f0-9]{40})\s+refs/tags/(v[0-9]\.[0-9]\.[0-9])$',
            re.MULTILINE,
        )

        releases = releases_expr.findall(content)

        release_branch_expr = re.compile(
            r'^([a-f0-9]{40})\s+refs/heads/(v[0-9]\.[0-9]\-branch)$',
            re.MULTILINE,
        )

        releases += release_branch_expr.findall(content)

        main_expr = re.compile(
            r'^([a-f0-9]{40})\s+refs/heads/(main)$',
            re.MULTILINE,
        )

        releases += main_expr.findall(content)

        return [rel for rel in releases if self._filter_tag_(rel[1], from_tag)]

    def _ncs_shas_to_zephyr_shas_(self, releases: list[tuple[str, str]]) -> list[tuple[str, str]]:
        return [
            (self._resolve_zephyr_sha_(self._get_zephyr_rev_(rel[0])), rel[1]) for rel in releases
        ]

    def _subject_is_in_zephyr_tree_(self, ref: str, subject: str) -> bool:
        shas = self._run_cmd_(
            self.zephyr_abspath,
            'git',
            'log',
            '--grep',
            subject,
            '-n',
            '1',
            '--format=%H',
            ref,
        )

        shas = shas.splitlines()

        if len(shas) == 0:
            return False

        if len(shas) > 1:
            raise CommandError(f'subject {subject} matches multiple shas: {shas}')

        return True

    def _get_zephyr_rev_(self, ref: str) -> str:
        west_yml = self._run_cmd_(self.nrf_abspath, 'git', 'show', f'{ref}:west.yml')
        content = yaml.safe_load(west_yml)
        projects = content['manifest']['projects']
        project = [p for p in projects if p['name'] == 'zephyr'][0]
        return project['revision']

    def _is_ncs_release_branch_(self, tag: str) -> bool:
        if tag == 'main':
            return True

        return re.fullmatch(r'v[0-9]+\.[0-9]-branch', tag) is not None

    def _format_ncs_timestamp_table_(
        self,
        ncs_timestamp_data: list[list[str]],
        ncs_timestamps: list[list[int | None]],
        subjects: list[str],
    ) -> str:
        headers = ['NCS tag', 'Zephyr rev'] + list(range(len(subjects)))
        rows = []

        for data_row, timestamp_row in zip(ncs_timestamp_data, ncs_timestamps, strict=True):
            rows.append(
                [data_row[0], data_row[1]]
                + [str(ts) if ts is not None else '-' for ts in timestamp_row]
            )

        return tabulate.tabulate(rows, headers=headers)

    def _resolve_ncs_timestamps_(
        self,
        ncs_nrf_releases: list[tuple[str, str]],
        ncs_releases: list[tuple[str, str]],
        ncs_data: list[list[str]],
        subjects: list[str],
    ) -> tuple[list[list[str]], list[list[int | None]]]:
        ncs_timestamp_releases = [r for r in ncs_nrf_releases if self._is_ncs_release_branch_(r[1])]
        ncs_timestamp_zephyr_releases = [
            r for r in ncs_releases if self._is_ncs_release_branch_(r[1])
        ]
        ncs_timestamp_data = [d for d in ncs_data if self._is_ncs_release_branch_(d[0])]
        ncs_timestamps: list[list[int | None]] = []

        for (nrf_sha, _), (zephyr_sha, _), row in zip(
            ncs_timestamp_releases, ncs_timestamp_zephyr_releases, ncs_timestamp_data, strict=True
        ):
            release_timestamps: list[int | None] = []

            for subject_index, subject in enumerate(subjects):
                if row[2 + subject_index] != 'Yes':
                    release_timestamps.append(None)
                    continue

                commit_sha = self._run_cmd_(
                    self.zephyr_abspath,
                    'git',
                    'log',
                    '--grep',
                    subject,
                    '-n',
                    '1',
                    '--format=%H',
                    zephyr_sha,
                ).strip()

                timestamp = self._timestamp_from_west_yml_(nrf_sha, commit_sha)
                if timestamp is None:
                    raise CommandError(
                        f'{row[0]}: commit present ({subject}) but west.yml availability '
                        f'timestamp could not be determined'
                    )
                release_timestamps.append(timestamp)

            ncs_timestamps.append(release_timestamps)

        return ncs_timestamp_data, ncs_timestamps

    def _timestamp_from_west_yml_(self, nrf_ref: str, commit_sha: str) -> int | None:
        log = self._run_cmd_(
            self.nrf_abspath,
            'git',
            'log',
            '--format=%H:%ct',
            nrf_ref,
            '--',
            'west.yml',
        )

        timestamp: int | None = None

        for entry in log.splitlines():
            if not entry:
                continue

            nrf_commit, ts_str = entry.split(':', maxsplit=1)

            zephyr_rev = self._get_zephyr_rev_(nrf_commit)
            zephyr_sha = self._resolve_zephyr_sha_(zephyr_rev)
            merge_base = self._run_cmd_(
                self.zephyr_abspath,
                'git',
                'merge-base',
                commit_sha,
                zephyr_sha,
            ).strip()

            if merge_base != commit_sha:
                break

            timestamp = int(ts_str)

        return timestamp
