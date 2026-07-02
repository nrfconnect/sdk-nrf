# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

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

        self.inf('')
        self.inf('Fetching NCS releases')
        ncs_releases = self._get_releases_(self.nrf_upstream_url, self.args.from_ncs_tag)
        self._fetch_releases_(self.nrf_abspath, self.nrf_upstream_url, ncs_releases)
        ncs_releases = self._ncs_shas_to_zephyr_shas_(ncs_releases)
        self._fetch_releases_(self.zephyr_abspath, self.zephyr_upstream_url, ncs_releases)
        ncs_data = self._check_releases_(ncs_releases, subjects, 'Checking NCS releases')

        ncs_headers = ['NCS tag', 'Zephyr rev'] + list(range(len(subjects)))
        ncs_table = tabulate.tabulate(ncs_data, headers=ncs_headers)

        self.inf('')
        self.inf('Fetching zephyr releases')
        zephyr_releases = self._get_releases_(self.zephyr_upstream_url, self.args.from_zephyr_tag)
        self._fetch_releases_(self.zephyr_abspath, self.zephyr_upstream_url, zephyr_releases)
        zephyr_data = self._check_releases_(zephyr_releases, subjects, 'Checking zephyr releases')

        zephyr_headers = ['Zephyr tag', 'Zephyr rev'] + list(range(len(subjects)))
        zephyr_table = tabulate.tabulate(zephyr_data, headers=zephyr_headers)

        self.inf('')
        self.inf('Commit overview')
        for i, subject in enumerate(subjects):
            self.inf(f'  {i}: {subject}')

        self.inf('')
        self.inf(ncs_table)

        self.inf('')
        self.inf(zephyr_table)

    def _validate_args_(self):
        version_expr = re.compile(r'^v[0-9]+\.[0-9]+$')

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
    ) -> list[list[str]]:
        self.inf('')
        self.inf(checking_label)

        data = []

        for sha, tag in releases:
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
