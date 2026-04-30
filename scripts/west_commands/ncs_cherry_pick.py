# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import re
import subprocess
from argparse import ArgumentParser, Namespace
from typing import Any

from west.commands import CommandError, WestCommand


class Repo:
    def __init__(
        self,
        upstream_url: str,
        downstream_url: str,
        cwd: str,
        branch: str,
        upstream_branch: str,
        downstream_branch: str,
    ):
        self.upstream_url = upstream_url
        self.downstream_url = downstream_url
        self.cwd = cwd
        self.branch = branch

        self.upstream_head_sha = self._get_remote_head_sha_(self.upstream_url, upstream_branch)
        self.downstream_head_sha = self._get_remote_head_sha_(
            self.downstream_url, downstream_branch
        )
        self.merge_base_sha = self._get_merge_base_(
            self.upstream_head_sha, self.downstream_head_sha
        )

        self.upstream_log = self._parse_commit_range_(self.merge_base_sha, self.upstream_head_sha)
        self.upstream_log_sha_map = {commit['sha']: commit for commit in self.upstream_log}
        self.downstream_log = self._parse_commit_range_(
            self.merge_base_sha, self.downstream_head_sha
        )
        self.branch_log = self._get_branch_log_()

        self.branch_len = 0

    def is_clean(self) -> bool:
        return not self._get_status_()

    def get_upstream_log(self) -> list[dict]:
        return self.upstream_log

    def get_downstream_log(self) -> list[dict]:
        return self.downstream_log

    def get_branch_log(self) -> list[dict]:
        return self.branch_log

    def get_log_from_upstream_shas(self, shas: list[str]) -> list[dict]:
        log = []

        for sha in shas:
            commit = self.upstream_log_sha_map.get(sha)

            if commit is None:
                raise CommandError(
                    f'Commit {sha} could not be found in upstream since latest upmerge.'
                )

            log.append(commit)

        return log

    def get_upstream_pr_log(self, pr_number: int) -> list[dict]:
        pr_head_sha = self._get_remote_head_sha_(self.upstream_url, f'pull/{pr_number}/head')
        pr_base_sha = self._get_merge_base_(self.upstream_head_sha, pr_head_sha)

        if pr_base_sha not in self.upstream_log_sha_map:
            raise CommandError(f'PR number {pr_number} is from before latest upmerge')

        pr_log = self._parse_commit_range_(pr_base_sha, pr_head_sha)

        # Track which PR the commits came from. Used to amend the commit message when cherry
        # picked.
        for commit in pr_log:
            commit['pr-number'] = pr_number

        return pr_log

    def reset_branch(self):
        self._checkout_ref_(self.downstream_head_sha)

        if self.branch in self._get_branches_():
            self._remove_branch_(self.branch)

        self._create_branch_()
        self._checkout_branch_()
        self.branch_len = 0

    def move_branch(self, index: int):
        if index > self.branch_len - 1 or index < 0:
            raise IndexError()

        delta = -(index + 1 - self.branch_len)
        self._reset_to_ref_(f'HEAD~{delta}')
        self.branch_len = index + 1

    def revert_commit(self, commit: dict) -> tuple[bool, list[str]]:
        try:
            self._run_cmd_('git', 'revert', '-s', '--no-edit', commit['sha'])
        except Exception:
            unmerged_paths = self._get_unmerged_paths_()
            self._run_cmd_('git', 'revert', '--abort')
            return False, unmerged_paths

        self.branch_len += 1
        return True, []

    def cherry_pick(self, commit: dict) -> tuple[bool, list[str]]:
        try:
            self._run_cmd_('git', 'cherry-pick', commit['sha'])
        except Exception:
            unmerged_paths = self._get_unmerged_paths_()
            self._run_cmd_('git', 'cherry-pick', '--abort')
            return False, unmerged_paths

        if self._commit_is_fromtree_(commit):
            self._amend_fromtree_commit_msg_(commit)
        elif self._commit_is_fromlist_(commit):
            self._amend_fromlist_commit_msg_(commit)
        elif not self._commit_is_noup_(commit):
            raise Exception(f'Commit {commit} is not a fromtree, fromlist nor noup commit')

        self.branch_len += 1
        return True, []

    def _get_status_(self) -> list[str]:
        return self._run_cmd_('git', 'status', '--porcelain').splitlines()

    def _get_unmerged_paths_(self) -> list[str]:
        expr = re.compile('^(U\\D|\\DU) ')
        lines = self._get_status_()
        return [line[3:] for line in lines if expr.search(line) is not None]

    def _amend_commit_msg_(self, fmt: str):
        msg = self._run_cmd_('git', 'log', f'--format={fmt}', '-n', '1')
        self._run_cmd_('git', 'commit', '--amend', '-m', msg)

    def _amend_fromtree_commit_msg_(self, commit: dict):
        self._amend_commit_msg_(
            f'[nrf fromtree] %s %n%n%b%n(cherry picked from commit {commit["sha"]})'
        )

    def _amend_fromlist_commit_msg_(self, commit: dict):
        self._amend_commit_msg_(f'[nrf fromlist] %s %n%n%b%nUpstream PR #: {commit["pr-number"]}')

    def _commit_is_fromtree_(self, commit: dict) -> bool:
        return 'sauce' not in commit and 'pr-number' not in commit

    def _commit_is_fromlist_(self, commit: dict) -> bool:
        return 'sauce' not in commit and 'pr-number' in commit

    def _commit_is_noup_(self, commit: dict) -> bool:
        return 'sauce' in commit and commit['sauce'] == 'nrf noup'

    def _get_branch_log_(self) -> list[dict]:
        if self.branch not in self._get_branches_():
            return []

        branch_head_sha = self._run_cmd_('git', 'rev-parse', self.branch).strip()
        merge_base_sha = self._get_merge_base_(self.downstream_head_sha, branch_head_sha)
        return self._parse_commit_range_(merge_base_sha, branch_head_sha)

    def _create_branch_(self):
        self._run_cmd_('git', 'branch', self.branch)

    def _checkout_branch_(self):
        self._run_cmd_('git', 'checkout', self.branch)

    def _get_branches_(self) -> list[str]:
        return self._run_cmd_('git', 'branch', '--format=%(refname:short)').splitlines()

    def _remove_branch_(self, branch: str):
        self._run_cmd_('git', 'branch', '-D', branch)

    def _checkout_ref_(self, ref: str):
        self._run_cmd_('git', 'checkout', ref)

    def _reset_to_ref_(self, ref: str):
        self._run_cmd_('git', 'reset', '--hard', ref)

    def _run_cmd_(self, cmd: str, *args) -> str:
        args = [cmd] + list(args)

        result = subprocess.run(args, capture_output=True, cwd=self.cwd)

        if result.returncode != 0:
            raise Exception(' '.join(args))

        return result.stdout.decode()

    def _get_head_sha_(self, ref: str) -> str:
        return self._run_cmd_('git', 'log', '--format=%H', '-n', '1', ref).strip()

    def _fetch_remote_ref_(self, remote: str, ref: str):
        self._run_cmd_('git', 'fetch', remote, ref)

    def _get_remote_head_sha_(self, remote: str, ref: str) -> str:
        self._fetch_remote_ref_(remote, ref)
        return self._get_head_sha_('FETCH_HEAD')

    def _get_merge_base_(self, ref_a: str, ref_b: str) -> str:
        return self._run_cmd_('git', 'merge-base', ref_a, ref_b).strip()

    def _parse_log_output_(self, output: str) -> list[dict]:
        revert_expr = re.compile(r'This reverts commit (\w{40})\.')
        sauce_tag_expr = re.compile(r'^\[(.+)\] (.+)$')
        pr_number_expr = re.compile(r'Upstream PR #: (\d+)')
        rename_expr = re.compile(r'\{(.*?) => (.*?)\}')
        bare_rename_expr = re.compile(r'^(.+?) => (.+)$')
        cherry_picked_expr = re.compile(r'^\(cherry picked from commit (\w{40})\)$', re.MULTILINE)

        if not output.strip():
            return []

        parts = output.split('\x00')[1:]
        commits = []

        for i in range(0, len(parts), 4):
            sha = parts[i]
            subject = parts[i + 1]
            date = int(parts[i + 2])
            body = parts[i + 3]
            body_and_files = body.strip().split('\n')

            revert_match = revert_expr.search(body)
            cherry_picked_match = cherry_picked_expr.search(body)

            files = []
            for line in body_and_files:
                if not line or '\t' not in line:
                    continue
                columns = line.split('\t')
                if len(columns) != 3:
                    continue
                try:
                    added = int(columns[0]) if columns[0] != '-' else 0
                    deleted = int(columns[1]) if columns[1] != '-' else 0
                except ValueError:
                    continue
                lines = added + deleted
                rename_match = rename_expr.search(columns[2])
                bare_rename_match = bare_rename_expr.match(columns[2])
                if rename_match:
                    old_path = (
                        columns[2][: rename_match.start()]
                        + rename_match.group(1)
                        + columns[2][rename_match.end() :]
                    ).replace('//', '/')
                    new_path = (
                        columns[2][: rename_match.start()]
                        + rename_match.group(2)
                        + columns[2][rename_match.end() :]
                    ).replace('//', '/')
                    files.append({'file': old_path, 'lines': lines})
                    files.append({'file': new_path, 'lines': lines})
                elif bare_rename_match:
                    files.append({'file': bare_rename_match.group(1), 'lines': lines})
                    files.append({'file': bare_rename_match.group(2), 'lines': lines})
                else:
                    files.append({'file': columns[2], 'lines': lines})

            commit = {
                'sha': sha,
                'subject': subject,
                'files': files,
                'date': date,
                'order': i >> 2,
            }

            if revert_match:
                commit['reverts'] = revert_match.group(1)

            if cherry_picked_match:
                commit['from'] = cherry_picked_match.group(1)

            tag_match = sauce_tag_expr.match(subject)
            if tag_match:
                commit['sauce'] = tag_match.group(1)
                commit['subject'] = tag_match.group(2)

            pr_match = pr_number_expr.search(body)
            if pr_match:
                commit['pr-number'] = int(pr_match.group(1))

            commits.append(commit)

        return commits

    def _parse_commit_range_(self, from_ref: str, to_ref: str) -> list[dict]:
        output = self._run_cmd_(
            'git', 'log', '--format=%x00%H%x00%s%x00%ct%x00%b', '--numstat', f'{from_ref}..{to_ref}'
        )
        return self._parse_log_output_(output)


class FakeRepo:
    def __init__(self):
        # Fake SHAs are 40 hex digits. The leading four digits encode GGNN.
        # GG = log group, NN = index within that log. The following 36 digits are zero.
        #
        # Log groups:
        #   00 upstream
        #   01 downstream
        #   02 upstream_pr_1
        #   03 upstream_pr_2
        #   04 branch
        #   05 upstream_pr_0
        self.logs = {
            'upstream': [
                {
                    # Reverts 0005. Filtered with it. Immutable (must not be picked/reverted
                    # in FakeRepo).
                    'sha': '0006000000000000000000000000000000000000',
                    'subject': 'Revert "trial change on d"',
                    'reverts': '0005000000000000000000000000000000000000',
                    'files': [
                        {
                            'file': 'd',
                            'lines': 3,
                            'depends': [
                                '0005000000000000000000000000000000000000',
                            ],
                        },
                    ],
                    'immutable': True,
                },
                {
                    # Reverted by 0006. Filtered with it. Immutable (must not be picked/reverted
                    # in FakeRepo).
                    'sha': '0005000000000000000000000000000000000000',
                    'subject': 'trial change on d',
                    'files': [
                        {
                            'file': 'd',
                            'lines': 3,
                            'depends': [
                                '0000000000000000000000000000000000000000',
                            ],
                        },
                    ],
                    'immutable': True,
                },
                {
                    # Touches a+b. Depends on 0002. Drives downstream conflict + dep order with
                    # 0004.
                    'sha': '0004000000000000000000000000000000000000',
                    'subject': 'modify a more and add to b',
                    'files': [
                        {
                            'file': 'a',
                            'lines': 10,
                            'depends': [
                                '0002000000000000000000000000000000000000',
                            ],
                        },
                        {
                            'file': 'b',
                            'lines': 5,
                        },
                    ],
                },
                {
                    # Adds file c. Paired with PR2 / branch fromlist-fromtree chain.
                    'sha': '0003000000000000000000000000000000000000',
                    'subject': 'create c',
                    'files': [
                        {
                            'file': 'c',
                            'lines': 8,
                        },
                    ],
                },
                {
                    # Modifies a. Listed in downstream noup conflicts. Depends on merge base 0000.
                    'sha': '0002000000000000000000000000000000000000',
                    'subject': 'modify a',
                    'files': [
                        {
                            'file': 'a',
                            'lines': 15,
                            'depends': [
                                '0000000000000000000000000000000000000000',
                            ],
                        },
                    ],
                },
                {
                    # Modifies b. Depends on merge base only.
                    'sha': '0001000000000000000000000000000000000000',
                    'subject': 'modify b',
                    'files': [
                        {
                            'file': 'b',
                            'lines': 3,
                            'depends': [
                                '0000000000000000000000000000000000000000',
                            ],
                        },
                    ],
                },
                {
                    # Merge-base/root. Satisfies file deps and downstream from=0000.
                    'sha': '0000000000000000000000000000000000000000',
                    'subject': 'create a and b and d',
                    'files': [
                        {
                            'file': 'a',
                            'lines': 6,
                        },
                        {
                            'file': 'b',
                            'lines': 11,
                        },
                        {
                            'file': 'd',
                            'lines': 22,
                        },
                    ],
                },
            ],
            'downstream': [
                {
                    # Pair with 0102. Filtered with it by _filter_reverted_commits_.
                    'sha': '0103000000000000000000000000000000000000',
                    'subject': 'Revert "[nrf noup] trial downstream change on d"',
                    'reverts': '0102000000000000000000000000000000000000',
                    'files': [
                        {
                            'file': 'd',
                            'lines': 2,
                            'depends': [
                                '0102000000000000000000000000000000000000',
                            ],
                        },
                    ],
                    'immutable': True,
                },
                {
                    # Noup on d. Reverted by 0103. Exercises immutable + revert pair on
                    # downstream.
                    'sha': '0102000000000000000000000000000000000000',
                    'subject': 'trial downstream change on d',
                    'sauce': 'nrf noup',
                    'files': [
                        {
                            'file': 'd',
                            'lines': 2,
                            'depends': [
                                '0000000000000000000000000000000000000000',
                            ],
                        },
                    ],
                    'immutable': True,
                },
                {
                    # Noup on a. Conflicts with cherry-picking upstream 0002/0004 (FakeRepo
                    # paths).
                    'sha': '0101000000000000000000000000000000000000',
                    'subject': 'modify a more downstream',
                    'sauce': 'nrf noup',
                    'files': [
                        {
                            'file': 'a',
                            'lines': 4,
                            'depends': [
                                '0000000000000000000000000000000000000000',
                            ],
                            'conflicts': [
                                '0004000000000000000000000000000000000000',
                                '0002000000000000000000000000000000000000',
                            ],
                        },
                    ],
                },
                {
                    # Baseline fromtree for 0000. Immutable. Supplies merge-base deps for
                    # noups.
                    'sha': '0100000000000000000000000000000000000000',
                    'sauce': 'nrf fromtree',
                    'from': '0000000000000000000000000000000000000000',
                    'subject': 'create a and b and d',
                    'files': [
                        {
                            'file': 'a',
                            'lines': 6,
                        },
                        {
                            'file': 'b',
                            'lines': 11,
                        },
                        {
                            'file': 'd',
                            'lines': 22,
                        },
                    ],
                    'immutable': True,
                },
            ],
            'upstream_pr_0': [
                {
                    # PR0. Contains 0003 cherry picked to branch as fromlist to simulate
                    # fromlist promotion to fromtree on rebase.
                    'sha': '0500000000000000000000000000000000000000',
                    'pr-number': 0,
                    'subject': 'create c',
                    'files': [
                        {
                            'file': 'c',
                            'lines': 8,
                        },
                    ],
                },
            ],
            'upstream_pr_1': [
                {
                    # Second commit on PR1. Depends on 0200 inside the PR.
                    'sha': '0201000000000000000000000000000000000000',
                    'pr-number': 1,
                    'subject': 'modify a and b more more',
                    'files': [
                        {
                            'file': 'a',
                            'lines': 2,
                            'depends': [
                                '0200000000000000000000000000000000000000',
                            ],
                        },
                        {
                            'file': 'b',
                            'lines': 11,
                            'depends': [
                                '0200000000000000000000000000000000000000',
                            ],
                        },
                    ],
                },
                {
                    # First PR1 commit. Depends on upstream 0004 (cross-log fromlist ordering).
                    'sha': '0200000000000000000000000000000000000000',
                    'pr-number': 1,
                    'subject': 'modify a and b more',
                    'files': [
                        {
                            'file': 'a',
                            'lines': 6,
                            'depends': [
                                '0004000000000000000000000000000000000000',
                            ],
                        },
                        {
                            'file': 'b',
                            'lines': 55,
                            'depends': [
                                '0004000000000000000000000000000000000000',
                            ],
                        },
                    ],
                },
            ],
            'upstream_pr_2': [
                {
                    # PR2. Depends on upstream 0003 (file c). Pairs with branch fromlist.
                    'sha': '0300000000000000000000000000000000000000',
                    'pr-number': 2,
                    'subject': 'modify c',
                    'files': [
                        {
                            'file': 'c',
                            'lines': 44,
                            'depends': [
                                '0003000000000000000000000000000000000000',
                            ],
                        },
                    ],
                },
            ],
            'branch': [
                {
                    # Noup reapplied last. Depends on 0002. Mirrors 0101-style noup on a.
                    'sha': '0404000000000000000000000000000000000000',
                    'sauce': 'nrf noup',
                    'subject': 'modify a more downstream',
                    'files': [
                        {
                            'file': 'a',
                            'lines': 4,
                            'depends': [
                                '0002000000000000000000000000000000000000',
                            ],
                        },
                    ],
                },
                {
                    # Fromlist for PR2. Matches upstream_pr_2 subject/lines for promotion tests.
                    'sha': '0403000000000000000000000000000000000000',
                    'subject': 'modify c',
                    'sauce': 'nrf fromlist',
                    'pr-number': 2,
                    'files': [
                        {
                            'file': 'c',
                            'lines': 44,
                        },
                    ],
                },
                {
                    # Fromlist cherry-pick of upstream 0003 (create c).
                    # Expected to be promoted to fromtree when rebased.
                    'sha': '0402000000000000000000000000000000000000',
                    'subject': 'create c',
                    'sauce': 'nrf fromlist',
                    'pr-number': 0,
                    'files': [
                        {
                            'file': 'c',
                            'lines': 8,
                        },
                    ],
                },
                {
                    # Fromlist cherry-pick of upstream 0002 (modify a).
                    'sha': '0401000000000000000000000000000000000000',
                    'subject': 'modify a',
                    'sauce': 'nrf fromtree',
                    'from': '0002000000000000000000000000000000000000',
                    'files': [
                        {
                            'file': 'a',
                            'lines': 15,
                        },
                    ],
                },
                {
                    # Reverts downstream noup 0101. Supplies reverts metadata on branch log.
                    'sha': '0400000000000000000000000000000000000000',
                    'subject': 'Revert modify a more downstream',
                    'reverts': '0101000000000000000000000000000000000000',
                    'files': [
                        {
                            'file': 'a',
                            'lines': 4,
                        },
                    ],
                },
            ],
        }

        for i, log in enumerate([kv[1] for kv in self.logs.items()]):
            for u, commit in enumerate(log):
                commit['order'] = u
                commit['date'] = (i * 10000) + ((len(log) - u - 1) * 60)

        self.upstream_log_sha_map = {commit['sha']: commit for commit in self.logs['upstream']}

        self.applied: list[tuple[str, dict]] = []
        self.branch_len = 0

    def is_clean(self) -> bool:
        return True

    def get_upstream_log(self) -> list[dict]:
        return self.logs['upstream']

    def get_downstream_log(self) -> list[dict]:
        return self.logs['downstream']

    def get_branch_log(self) -> list[dict]:
        return self.logs['branch']

    def get_log_from_upstream_shas(self, shas: list[str]) -> list[dict]:
        return [self.upstream_log_sha_map[sha] for sha in shas]

    def get_upstream_pr_log(self, pr_number: int) -> list[dict]:
        return self.logs[f'upstream_pr_{pr_number}']

    def reset_branch(self):
        self.applied = []
        self.branch_len = 0

    def move_branch(self, index: int):
        if index > self.branch_len - 1 or index < 0:
            raise IndexError()

        self.applied = self.applied[: index + 1]
        self.branch_len = len(self.applied)

    def get_applied(self) -> list[tuple[str, dict]]:
        return self.applied

    def _commit_is_fromtree_(self, commit: dict) -> bool:
        return 'sauce' not in commit and 'pr-number' not in commit

    def _commit_is_fromlist_(self, commit: dict) -> bool:
        return 'sauce' not in commit and 'pr-number' in commit

    def _commit_is_noup_(self, commit: dict) -> bool:
        return 'sauce' in commit and commit['sauce'] == 'nrf noup'

    def _reverted_commit_shas_(self) -> set[str]:
        return {c['sha'] for action, c in self.applied if action == 'revert'}

    def _upstream_dep_satisfied_(self, dep_sha: str) -> bool:
        if any(action == 'cherry-pick' and c['sha'] == dep_sha for action, c in self.applied):
            return True

        return any(c.get('from') == dep_sha for c in self.logs['downstream'])

    def _cherry_pick_upstream_dep_conflict_paths_(self, commit: dict) -> list[str]:
        paths: list[str] = []
        for f in commit.get('files', []):
            if 'depends' not in f:
                continue
            for dep_sha in f['depends']:
                if not self._upstream_dep_satisfied_(dep_sha):
                    paths.append(f['file'])
                    break
        return paths

    def _cherry_pick_downstream_conflict_paths_(self, commit: dict) -> list[str]:
        reverted = self._reverted_commit_shas_()
        paths: list[str] = []
        for d in self.logs['downstream']:
            if d['sha'] in reverted:
                continue
            for f in d['files']:
                if 'conflicts' not in f:
                    continue
                if commit['sha'] in f['conflicts']:
                    paths.append(f['file'])
        return paths

    def _reject_if_immutable_(self, commit: dict, operation: str) -> None:
        if commit.get('immutable') is True:
            raise CommandError(
                f'commit {commit["sha"][:12]}... is immutable and cannot be {operation}'
            )

    def revert_commit(self, commit: dict) -> tuple[bool, list[str]]:
        self._reject_if_immutable_(commit, 'reverted')
        self.applied.append(('revert', commit))
        self.branch_len = len(self.applied)
        return True, []

    def cherry_pick(self, commit: dict) -> tuple[bool, list[str]]:
        self._reject_if_immutable_(commit, 'cherry-picked')
        if not (
            self._commit_is_fromtree_(commit)
            or self._commit_is_fromlist_(commit)
            or self._commit_is_noup_(commit)
        ):
            raise Exception(f'{commit} is not a fromtree, fromlist nor noup commit')

        upstream_dep_paths = self._cherry_pick_upstream_dep_conflict_paths_(commit)
        if upstream_dep_paths:
            return False, upstream_dep_paths

        conflict_paths = self._cherry_pick_downstream_conflict_paths_(commit)
        if conflict_paths:
            return False, conflict_paths

        self.applied.append(('cherry-pick', commit))
        self.branch_len = len(self.applied)
        return True, []


class NcsCherryPick(WestCommand):
    def __init__(self) -> None:
        super().__init__(
            name='ncs-cherry-pick',
            help='NCS cherry-pick utility',
            description='Helps cherry pick commits from OSS projects to a local branch',
        )

        # Manually updated list of URLs pointing to upstreams of projects
        self.upstream_urls = {
            'zephyr': 'https://github.com/zephyrproject-rtos/zephyr',
            'mcuboot': 'https://github.com/zephyrproject-rtos/mcuboot',
        }

    def do_add_parser(self, parser_adder: Any) -> ArgumentParser:
        parser = parser_adder.add_parser(self.name, help=self.help, description=self.description)

        parser.add_argument(
            'branch',
            help='Name to use for the local branch which will store the cherry-picked commits',
            type=str,
        )

        parser.add_argument(
            '--upstream-branch',
            help='Name of the upstream branch to cherry pick from',
            type=str,
            default='main',
        )

        parser.add_argument(
            '--downstream-branch',
            help='Name of the downstream branch to cherry pick onto',
            type=str,
            default='main',
        )

        parser.add_argument(
            '--commit',
            help='Hash of commit from OSS upstream to include in cherry-pick',
            type=str,
            action='append',
            dest='commits',
        )

        parser.add_argument(
            '--pr-number',
            help='Pull request number of pull request to OSS upstream to include in cherry-pick',
            type=int,
            action='append',
            dest='pr_numbers',
        )

        parser.add_argument(
            '--project',
            help='OSS project name as defined in the west manifest (zephyr, mcuboot, ...)',
            type=str,
            default='zephyr',
            choices=[k for k in self.upstream_urls],
        )

        parser.add_argument(
            '-p',
            '--pristine',
            help='Delete existing branch dropping all commits',
            action='store_true',
        )

        parser.add_argument(
            '--limit',
            help='Artifically limit the number of commits on the cherry pick branch',
            default=60,
            type=int,
        )

        parser.add_argument('--test', help='Use fake local repo for testing', action='store_true')

        return parser

    def do_run(self, args: Namespace, _: list[str]) -> None:
        self.args = args

        if self.args.commits:
            self._validate_commits_arg_()

        if self.args.pr_numbers:
            self._validate_pr_numbers_arg_()

        self.fromlist_shas = []
        self.fromlist_pr_map = {}
        self.fromtree_shas = []
        self.noup_shas = []
        self.pr_numbers = []

        project = self.manifest.get_projects([args.project])[0]

        self.inf(f'Initializing {project.name} repository')
        if self.args.test:
            self.repo = FakeRepo()
        else:
            self.repo = Repo(
                self.upstream_urls[project.name],
                project.url,
                project.abspath,
                args.branch,
                args.upstream_branch,
                args.downstream_branch,
            )

        if not self.repo.is_clean():
            raise CommandError(f'The {project.name} git repo at {project.abspath} is not clean')

        self.upstream_log = self.repo.get_upstream_log()
        self.downstream_log = self.repo.get_downstream_log()

        self.upstream_log = self._filter_reverted_commits_(self.upstream_log)
        self.downstream_log = self._filter_reverted_commits_(self.downstream_log)

        # Fromlists get merged, dropped, or forgotten upstream after they where merged
        # downstream. We need to synchronize this with upstream.
        self._sync_downstream_fromlists_(self.downstream_log, self.upstream_log)

        self.upstream_file_index = self._build_file_index_(self.upstream_log)
        self.downstream_file_index = self._build_file_index_(self.downstream_log)

        if not self.args.pristine:
            # We will ensure every cherry picked commit currently present on the branch
            # will be present in the final cherry pick unless --pristine is used. This
            # allows for incrementally building up a cherry pick or rebasing a completed
            # cherry pick.
            self.branch_log = self.repo.get_branch_log()
        else:
            self.branch_log = []

        # Find every PR to include, both from --pr-number args and any fromlist PRs
        # currently present on the branch.
        pr_numbers = [c['pr-number'] for c in self.branch_log if 'pr-number' in c]
        if self.args.pr_numbers:
            pr_numbers += self.args.pr_numbers
        pr_numbers = list(set(pr_numbers))

        # Get commit log from every PR.
        self.fromlist_log = []
        if pr_numbers:
            self.inf('')
            self.inf('Fetching upstream pull requests')
        for pr_number in pr_numbers:
            self.inf(str(pr_number))
            self.fromlist_log += self.repo.get_upstream_pr_log(pr_number)

        # Get every fromtree commit to include in the cherry pick, both from --commit
        # args and any fromtree commits currently present on the branch.
        fromtree_shas = [c['from'] for c in self.branch_log if c.get('sauce') == 'nrf fromtree']
        if self.args.commits:
            fromtree_shas += self.args.commits
        self.fromtree_log = self.repo.get_log_from_upstream_shas(fromtree_shas)

        # We will try to reapply the noups present on the branch on top of the cherry pick.
        # Reverted noups are will be automatically reapplied if they still conflict with
        # the cherry pick.
        self.noup_log = [c for c in self.branch_log if c.get('sauce') == 'nrf noup']

        # Commits which were cherry picked from PRs may now be merged upstream, find
        # and promote them to fromtrees.
        self.fromlist_log, self.fromtree_log = self._promote_fromlists_to_fromtrees_(
            self.fromlist_log, self.fromtree_log, self.upstream_log
        )

        # Ensure fromtrees are sorted as they can be provided in any order.
        self._sort_commit_log_(self.fromtree_log)

        # Filter out fromtree commits which are already present in downstream.
        self.fromtree_log = self._filter_already_present_fromtrees_(
            self.fromtree_log, self.downstream_log
        )

        if not self.fromtree_log and not self.fromlist_log:
            self.inf('Nothing to cherry pick')
            return

        self.inf('')
        self.inf('Performing cherry pick')

        self.repo.reset_branch()

        self.revert_log = []
        prev_plan = []
        progress = 0

        while True:
            plan = self._build_plan_()

            if len(plan) > self.args.limit:
                raise CommandError(f'Reached artificial --limit of {self.args.limit} commits.\n')

            if prev_plan:
                self.inf('Conflict')
                equal_count = self._compare_plans_(plan, prev_plan)
                if equal_count == 0:
                    self.inf('Reset branch')
                    self.repo.reset_branch()
                    progress = 0
                elif equal_count < progress:
                    self.inf(f'Move branch back by {progress - equal_count} commits')
                    self.repo.move_branch(equal_count - 1)
                    progress = equal_count

            prev_plan = plan

            commit = None
            applied = True
            unmerged_paths = []

            for action, commit in plan[progress:]:
                if action == 'revert':
                    self.inf(f'Reverting {self._describe_commit_(commit)}')
                    applied, unmerged_paths = self.repo.revert_commit(commit)
                else:
                    self.inf(f'Cherry picking {self._describe_commit_(commit)}')
                    applied, unmerged_paths = self.repo.cherry_pick(commit)

                if not applied:
                    if action == 'revert':
                        self._resolve_revert_conflict_(commit, unmerged_paths)
                    elif not unmerged_paths:
                        self._resolve_empty_cherry_pick_conflict_(commit)
                    else:
                        self._resolve_cherry_pick_conflict_(commit, unmerged_paths)
                    break

                progress += 1

            if applied:
                break

        # Try to reapply noup commits which where present on the branch before it was reset.
        for commit in reversed(self.noup_log):
            applied, _ = self.repo.cherry_pick(commit)
            if applied:
                self.inf(f'Reapplied {self._describe_commit_(commit)}')

                # Append the applied noup to the plan so it gets included in the summary.
                plan.append(('pick', commit))
            else:
                self.wrn(f'Failed to reapply {self._describe_commit_(commit)}')
                # Append the conflicting noup to the revert_log so it gets
                self.revert_log.append(commit)

        self.inf('')
        self.inf('Cherry pick summary')
        for action, commit in plan:
            self.inf(f'{action} {commit["sha"][:16]} # {commit["subject"]}')

        if self.revert_log:
            msg = 'The following commits must be cherry picked manually'
            self.inf('')
            if self.args.test:
                self.inf(msg)
            else:
                self.wrn(msg)

            for commit in reversed(
                [commit for commit in self.revert_log if commit['sauce'] == 'nrf noup']
            ):
                self.inf(f'pick {commit["sha"][:20]} # [{commit["sauce"]}] {commit["subject"]}')

    def _build_plan_(self) -> list[tuple[str, dict]]:
        plan = [('revert', c) for c in self.revert_log]
        plan += [('pick', c) for c in reversed(self.fromtree_log)]
        plan += [('pick', c) for c in reversed(self.fromlist_log)]
        return plan

    def _compare_plans_(
        self, plan_a: list[tuple[str, dict]], plan_b: list[tuple[str, dict]]
    ) -> int:
        min_len = min(len(plan_a), len(plan_b))

        for i in range(min_len):
            sha_a = plan_a[i][1]['sha']
            sha_b = plan_b[i][1]['sha']
            if sha_a != sha_b:
                return i

        return min_len

    def _validate_commits_arg_(self):
        commit_expr = re.compile(r'^\w{40}$')

        for sha in self.args.commits:
            if commit_expr.fullmatch(sha) is None:
                raise ValueError(f'--commit {sha} is invalid')

    def _validate_pr_numbers_arg_(self):
        for pr_number in self.args.pr_numbers:
            if pr_number < 0:
                raise ValueError(f'--pr-number {pr_number} is invalid')

    def _describe_commit_(self, commit: dict) -> str:
        if 'sauce' in commit:
            return f'{commit["sha"][:20]} [{commit["sauce"]}] {commit["subject"]}'
        else:
            return f'{commit["sha"][:20]} {commit["subject"]}'

    def _build_file_index_(self, commits: list[dict]) -> dict[str, list[dict]]:
        index: dict[str, list[dict]] = {}

        for commit in reversed(commits):
            for f in commit['files']:
                index.setdefault(f['file'], []).append(commit)

        return index

    def _filter_reverted_commits_(self, commits: list[dict]) -> list[dict]:
        sha_to_idx = {commit['sha']: i for i, commit in enumerate(commits)}

        revert_pairs = [
            (commit['sha'], commit['reverts'])
            for commit in commits
            if 'reverts' in commit and commit['reverts'] in sha_to_idx
        ]

        revert_pairs.sort(key=lambda p: sha_to_idx.get(p[1], -1), reverse=True)

        removed = set()
        for reverter_sha, reverted_sha in revert_pairs:
            if reverter_sha not in removed and reverted_sha not in removed:
                removed.add(reverter_sha)
                removed.add(reverted_sha)

        return [commit for commit in commits if commit['sha'] not in removed]

    def _find_commit_in_log_(self, commit: dict, commit_log: list[dict]) -> dict | None:
        for c in commit_log:
            if commit['subject'] != c['subject']:
                continue

            c_files = {(f['file'], f['lines']) for f in c['files']}
            commit_files = {(f['file'], f['lines']) for f in commit['files']}

            if c_files != commit_files:
                continue

            return c

        return None

    def _commit_is_in_log_(self, commit: dict, commit_log: list[dict]) -> bool:
        return self._find_commit_in_log_(commit, commit_log) is not None

    def _promote_fromlists_to_fromtrees_(
        self, fromlist_log: list[dict], fromtree_log: list[dict], upstream_log: list[dict]
    ) -> tuple[list[dict], list[dict]]:
        remaining_fromlist_log = []

        for commit in fromlist_log:
            similar_commit = self._find_commit_in_log_(commit, upstream_log)
            if similar_commit is None:
                remaining_fromlist_log.append(commit)
            else:
                fromtree_log.append(similar_commit)

        return remaining_fromlist_log, fromtree_log

    def _filter_already_present_fromtrees_(
        self, fromtree_log: list[dict], downstream_log: list[dict]
    ) -> list[dict]:
        return [c for c in fromtree_log if self._find_commit_in_log_(c, downstream_log) is None]

    def _sync_downstream_fromlists_(self, downstream_log: list[dict], upstream_log: list[dict]):
        for commit in downstream_log:
            if commit.get('sauce') != 'nrf fromlist':
                # Only fromlists can get out of sync.
                continue

            similar_commit = self._find_commit_in_log_(commit, upstream_log)
            if similar_commit is not None:
                # Fromlist has been merged and is now a fromtree. Update to fromtree.
                commit['sauce'] = 'nrf fromtree'
                commit['from'] = similar_commit['sha']
            else:
                # Fromlist has not been merged, at least not cleanly. Update to noup.
                commit['sauce'] = 'nrf noup'

            # PR number has no relevance for fromtree nor noups, remove it.
            commit.pop('pr-number')

    def _sort_commit_log_(self, commit_log: list[dict], reverse: bool = False):
        commit_log.sort(key=lambda c: c['order'], reverse=reverse)

    def _resolve_revert_conflict_(self, commit: dict, unmerged_paths: list[str]):
        # Get all downstream commits which touch the unmerged paths and are not
        # already being reverted
        revert_shas = [c['sha'] for c in self.revert_log]
        touching_shas = []
        touching_commits = []
        for p in unmerged_paths:
            cs = self.downstream_file_index[p]
            for c in cs:
                if c['sha'] in touching_shas or c['sha'] in revert_shas:
                    continue

                touching_shas.append(c['sha'])
                touching_commits.append(c)

        # Only revert touching commits which are newer than the conflicting commit since it
        # can't conflict with a commit merged before it. Touching commits and conflicting
        # commit are both from the same branch so we can compare the merge dates.
        touching_commits = [c for c in touching_commits if c['order'] < commit['order']]

        assert touching_commits, 'Failed to resolve revert conflict'

        # Revert the touching commit closest to HEAD, starting with noup commits
        self._sort_commit_log_(touching_commits)
        touching_noup_commits = [c for c in touching_commits if c['sauce'] == 'nrf noup']
        revert_commit = touching_noup_commits[0] if touching_noup_commits else touching_commits[0]

        # Add the commit to the revert list and sort it
        self.revert_log.append(revert_commit)
        self._sort_commit_log_(self.revert_log)

        # If the reverted commit is a fromtree, cherry pick it back in the correct order
        if revert_commit['sauce'] == 'nrf fromtree':
            reapply_commit = self.repo.get_log_from_upstream_shas([revert_commit['from']])[0]
            self.fromtree_log.append(reapply_commit)
            self._sort_commit_log_(self.fromtree_log)

    def _resolve_empty_cherry_pick_conflict_(self, commit: dict):
        # The commit being cherry picked has been uncleanly cherry picked to downstream
        # resulting in an empty cherry pick. Find the commit by looking at the files
        # touched by the commit being cherry picked and revert it.
        revert_shas = [c['sha'] for c in self.revert_log]
        touching_shas = []
        touching_commits = []
        for file in commit['files']:
            cs = self.downstream_file_index[file['file']]
            for c in cs:
                if c['sha'] in touching_shas or c['sha'] in revert_shas:
                    continue

                touching_shas.append(c['sha'])
                touching_commits.append(c)

        assert touching_commits, 'Failed to resolve empty cherry pick conflict'

        self._sort_commit_log_(touching_commits)
        self.revert_log.append(touching_commits[0])
        self._sort_commit_log_(self.revert_log)

    def _resolve_cherry_pick_conflict_(self, commit: dict, unmerged_paths: list[str]):
        # Get all upstream commits which touch the unmerged paths and are not already
        # being cherry picked
        fromtree_shas = [c['sha'] for c in self.fromtree_log]
        touching_shas = []
        touching_commits = []
        for p in unmerged_paths:
            cs = self.upstream_file_index[p]
            for c in cs:
                if c['sha'] in touching_shas or c['sha'] in fromtree_shas:
                    continue

                touching_shas.append(c['sha'])
                touching_commits.append(c)

        # Only cherry pick commits which are not already cherry picked downstream
        touching_commits = [
            c for c in touching_commits if not self._commit_is_in_log_(c, self.downstream_log)
        ]

        # Only cherry pick touching commits which are older than the conflicting commit since it
        # can't depend on a commit merged after it. Note that commits from PRs always on top of
        # upstream so they are always newer than any touching commit.
        if 'pr-number' not in commit:
            touching_commits = [c for c in touching_commits if c['order'] > commit['order']]

        if not touching_commits:
            # No missing commits from upstream, we need to revert a conflicting
            # downstream commit
            self._resolve_revert_conflict_(commit, unmerged_paths)
            return

        self._sort_commit_log_(touching_commits, reverse=True)
        self.fromtree_log.append(touching_commits[0])
        self._sort_commit_log_(self.fromtree_log)
