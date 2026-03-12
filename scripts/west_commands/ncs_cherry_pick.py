# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import subprocess
from argparse import Namespace, ArgumentParser
from west.commands import WestCommand
from typing import Any
import re

class NcsCherryPick(WestCommand):
    def __init__(self) -> None:
        super().__init__(
            name='ncs-cherry-pick',
            help='NCS cherry-pick utility',
            description='Helps cherry-pick commits from OSS projects to a local up-to-date branch',
        )

        # Manually updated list of URLs pointing to upstreams of projects
        self.upstream_urls = {
            'zephyr': 'https://github.com/zephyrproject-rtos/zephyr',
            'mcuboot': 'https://github.com/zephyrproject-rtos/mcuboot',
        }

    def do_add_parser(self, parser_adder: Any) -> ArgumentParser:
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            description=self.description
        )

        parser.add_argument(
            'branch',
            help='Name to use for the local branch which will store the cherry-picked commits',
            type=str,
        )

        parser.add_argument(
            '--commit',
            help='Hash of commit from OSS upstream to include in cherry-pick',
            type=str,
            action='append',
            dest='commits'
        )

        parser.add_argument(
            '--pr-number',
            help='Pull request number of pull request to OSS upstream to include in cherry-pick',
            type=int,
            action='append',
            dest='pr_numbers'
        )

        parser.add_argument(
            '--project',
            help='OSS project name as defined in the west manifest (zephyr, mcuboot, ...)',
            type=str,
            default='zephyr',
            choices=[k for k in self.upstream_urls]
        )

        parser.add_argument(
            '-p',
            '--pristine',
            help='Delete existing branch dropping all commits',
            action='store_true'
        )

        return parser

    def do_run(self, args: Namespace, _: list[str]) -> None:
        self.args = args

        if self.args.commits:
            self._validate_commits_arg_()

        if self.args.pr_numbers:
            self._validate_pr_numbers_arg_()

        self.project = self.manifest.get_projects([args.project])[0]

        self.upstream_url = self.upstream_urls[self.project.name]
        self.downstream_url = self.project.url

        self.fromlist_shas = []
        self.fromlist_pr_map = {}
        self.fromtree_shas = []
        self.noup_shas = []
        self.pr_numbers = []

        self.inf(f'Fetching {self.project.name} repository')
        self.upstream_main_sha = self._get_remote_head_sha_(
            self.upstream_url,
            'main'
        )
        self.downstream_main_sha = self._get_remote_head_sha_(
            self.downstream_url,
            'main'
        )
        self.merge_base_sha = self._get_merge_base_(
            self.upstream_main_sha,
            self.downstream_main_sha
        )

        self.inf('Parsing commit history')
        self.upstream_log = self._parse_commit_range_(
            self.merge_base_sha, self.upstream_main_sha
        )
        self.downstream_log = self._parse_commit_range_(
            self.merge_base_sha, self.downstream_main_sha
        )

        self.upstream_log = self._filter_reverted_commits_(self.upstream_log)
        self.upstream_log_sha_map = {commit['sha']: commit for commit in self.upstream_log}
        self.downstream_log = self._filter_reverted_commits_(self.downstream_log)
        self._update_downstream_tags_(self.downstream_log, self.upstream_log)
        self.upstream_file_index = self._build_file_index_(self.upstream_log)
        self.downstream_file_index = self._build_file_index_(self.downstream_log)

        if self.args.pristine or self.args.branch not in self._get_branches_():
            self.inf(f'Creating {self.args.branch} branch')
            self._checkout_ref_(self.downstream_main_sha)
            if self.args.branch in self._get_branches_():
                self._remove_branch_(self.args.branch)
            self._checkout_to_branch_(self.args.branch)
        else:
            self.inf(f'Reading {self.args.branch}')
            self._checkout_ref_(self.args.branch)
            self.fromtree_shas += self._get_forked_fromtree_shas_()
            self.noup_shas += self._get_forked_noup_shas_()
            self.pr_numbers += self._get_forked_pr_numbers_()
            self.inf(f'Resetting {self.args.branch}')
            self._reset_to_ref_(self.downstream_main_sha)

        if self.args.pr_numbers:
            for pr in self.args.pr_numbers:
                if pr not in self.pr_numbers:
                    self.pr_numbers.append(pr)

        for pr_number in self.pr_numbers:
            self.inf(f'Fetching upstream pull request number {pr_number}')
            pr_shas = self._get_pr_shas_(self.upstream_url, pr_number)
            for sha in pr_shas:
                self.fromlist_pr_map[sha] = pr_number
            self.fromlist_shas += pr_shas

        if self.args.commits:
            self.fromtree_shas += self.args.commits
            self.fromtree_shas = list(set(self.fromtree_shas))

        self.fromlist_log = self._parse_shas_(self.fromlist_shas)
        self.fromtree_log = self._parse_shas_(self.fromtree_shas)
        self.noup_log = self._parse_shas_(self.noup_shas)

        # Commits which were cherry picked from PRs may now be merged upstream, find
        # and promote them to fromtrees.
        self.fromlist_log, self.fromtree_log = self._promote_fromlists_to_fromtrees_(
            self.fromlist_log,
            self.fromtree_log,
            self.upstream_log
        )

        # Filter out fromtree commits which are already present in downstream.
        self.fromtree_log = self._filter_already_present_fromtrees_(
            self.fromtree_log,
            self.downstream_log
        )

        if not self.fromtree_log and not self.fromlist_log:
            self.inf('Nothing to cherry pick')
            return

        self.revert_log = []
        prev_plan = []
        progress = 0

        while True:
            plan = self._build_plan_()

            if prev_plan:
                index = self._compare_plans_(plan, prev_plan)
                if index < progress:
                    self.inf(f'Resetting {self.args.branch} branch back by {progress - index} commits')
                    self._reset_to_ref_(f'HEAD~{progress - index}')
                    progress = index

            prev_plan = plan

            commit = None
            applied = True
            unmerged_paths = []

            for action, commit in plan[progress:]:
                if action == 'revert':
                    applied, unmerged_paths = self._revert_commit_(commit)
                else:
                    applied, unmerged_paths = self._cherry_pick_(commit)

                if not applied:
                    if action == 'revert':
                        self._resolve_revert_conflict_(unmerged_paths)
                    elif not unmerged_paths:
                        self._resolve_empty_cherry_pick_conflict_(commit)
                    else:
                        self._resolve_cherry_pick_conflict_(unmerged_paths)
                    break

                progress += 1

            if applied:
                break

        if self.revert_log:
            self.wrn('The following commits caused conflicts and must be manually cherry picked:')
            self.inf(f'# git rebase -i')
            for commit in reversed(
                [commit for commit in self.revert_log if commit['sauce'] == 'nrf noup']
            ):
                self.inf(f'pick {commit["sha"][:20]} # [{commit["sauce"]}] {commit["subject"]}')

    def _run_cmd_(self, cmd: str, *args) -> str:
        result = subprocess.run(
            [cmd] + list(args),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=self.project.abspath
        )

        assert result.returncode == 0
        return result.stdout.decode().replace('\xa0', ' ')

    def _build_plan_(self) -> list[tuple[str, dict]]:
        plan = [('revert', c) for c in self.revert_log]
        plan += [('fromtree', c) for c in reversed(self.fromtree_log)]
        plan += [('fromlist', c) for c in reversed(self.fromlist_log)]
        return plan

    def _compare_plans_(self, plan_a: list[tuple[str, dict]], plan_b: list[tuple[str, dict]]) -> int:
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
            if commit_expr.search(sha) is None:
                raise ValueError(f'--commit {sha} is invalid')

    def _validate_pr_numbers_arg_(self):
        for pr_number in self.args.pr_numbers:
            if pr_number < 0:
                raise ValueError(f'--pr-number {pr_number} is invalid')

    def _fetch_remote_ref_(self, remote: str, ref: str):
        self._run_cmd_('git', 'fetch', remote, ref)

    def _checkout_ref_(self, ref: str):
        self._run_cmd_('git', 'checkout', ref)

    def _reset_to_ref_(self, ref: str):
        self._run_cmd_('git', 'reset', '--hard', ref)

    def _checkout_to_branch_(self, branch: str):
        self._run_cmd_('git', 'checkout', '-b', branch)

    def _get_head_sha_(self, ref: str) -> str:
        return self._run_cmd_('git', 'log', '--format=%H', '-n', '1', ref)[:-1]

    def _get_remote_head_sha_(self, remote: str, ref: str) -> str:
        self._fetch_remote_ref_(remote, ref)
        return self._get_head_sha_('FETCH_HEAD')

    def _get_branches_(self) -> list[str]:
        return self._run_cmd_('git', 'branch', '--format=%(refname:short)').splitlines()

    def _remove_branch_(self, branch: str):
        self._run_cmd_('git', 'branch', '-D', branch)

    def _get_merge_base_(self, ref_a: str, ref_b: str) -> str:
        return self._run_cmd_('git', 'merge-base', ref_a, ref_b)[:-1]

    def _get_sha_range_(self, from_ref: str, to_ref: str) -> list[str]:
        return self._run_cmd_('git', 'log', '--format=%H', f'{from_ref}..{to_ref}')[:-1].split('\n')

    def _describe_commit_(self, commit: dict) -> str:
        if 'sauce' in commit:
            return f'{commit["sha"][:20]} [{commit["sauce"]}] {commit["subject"]}'
        else:
            return f'{commit["sha"][:20]} {commit["subject"]}'

    def _get_sha_header_(self, sha: str) -> str:
        return self._run_cmd_('git', 'log', '--format=%s', '-n', '1', sha)[:-1]

    def _get_sha_body_(self, sha: str) -> str:
        return self._run_cmd_('git', 'log', '--format=%B', '-n', '1', sha)[:-1]

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
                    old_path = (columns[2][:rename_match.start()] + rename_match.group(1) + columns[2][rename_match.end():]).replace('//', '/')
                    new_path = (columns[2][:rename_match.start()] + rename_match.group(2) + columns[2][rename_match.end():]).replace('//', '/')
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
                'order': i >> 2 if i > 0 else 0,
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
            'git', 'log',
            '--format=%x00%H%x00%s%x00%ct%x00%b',
            '--numstat',
            f'{from_ref}..{to_ref}'
        )
        return self._parse_log_output_(output)

    def _parse_shas_(self, shas: list[str]) -> list[dict]:
        if not shas:
            return []
        output = self._run_cmd_(
            'git', 'log', '--no-walk=unsorted',
            '--format=%x00%H%x00%s%x00%ct%x00%b',
            '--numstat',
            *shas
        )
        return self._parse_log_output_(output)

    def _build_file_index_(self, commits: list[dict]) -> dict[str, list[dict]]:
        index: dict[str, list[dict]] = {}

        for commit in reversed(commits):
            for f in commit['files']:
                index.setdefault(f['file'], []).append(commit)

        return index

    def _filter_reverted_commits_(self, commits: list[dict]) -> list[dict]:
        sha_to_idx = {commit['sha']: i for i, commit in enumerate(commits)}

        revert_pairs = [
            (commit['sha'], commit['reverts']) for commit in commits
            if 'reverts' in commit and commit['reverts'] in sha_to_idx
        ]

        revert_pairs.sort(
            key=lambda p: sha_to_idx.get(p[1], -1), reverse=True
        )

        removed = set()
        for reverter_sha, reverted_sha in revert_pairs:
            if reverter_sha not in removed and reverted_sha not in removed:
                removed.add(reverter_sha)
                removed.add(reverted_sha)

        return [commit for commit in commits if commit['sha'] not in removed]

    def _get_sha_sauce_tag_(self, sha: str) -> str:
        subject = self._get_sha_header_(sha)
        subject_expr = re.compile('^(\\[[\\w\\s]+\\]) ([\\w\\s\\:\\./]+)$')
        matches = subject_expr.findall(subject)
        if matches:
            return matches[0][0]

        return ''

    def _get_pr_shas_(self, remote: str, pr_number: int) -> list[str]:
        pr_head_sha = self._get_remote_head_sha_(remote, f'pull/{pr_number}/head')
        pr_base_sha = self._get_merge_base_(self.upstream_main_sha, pr_head_sha)
        return self._get_sha_range_(pr_base_sha, pr_head_sha)

    def _get_forked_shas_(self) -> list[str]:
        head_sha = self._get_head_sha_('HEAD')

        if self.downstream_main_sha == head_sha:
            return []

        return self._get_sha_range_(self.downstream_main_sha, head_sha)

    def _get_forked_noup_shas_(self) -> list[str]:
        shas = self._get_forked_shas_()
        return [sha for sha in shas if self._get_sha_sauce_tag_(sha) == '[nrf noup]']

    def _get_forked_fromtree_shas_(self) -> list[str]:
        shas = self._get_forked_shas_()
        shas = [sha for sha in shas if self._get_sha_sauce_tag_(sha) == '[nrf fromtree]']
        expr = re.compile(r'\(cherry picked from commit (\w+)\)')
        upstream_shas = []
        for sha in shas:
            body = self._get_sha_body_(sha)
            matches = expr.findall(body)
            assert matches, (
                f'[nrf fromtree] commit {sha[:12]} is missing a '
                f'"(cherry picked from commit <upstream-sha>)" line in its message body'
            )
            upstream_shas.append(matches[0])
        return upstream_shas

    def _get_forked_pr_numbers_(self) -> list[int]:
        shas = self._get_forked_shas_()
        expr = re.compile('Upstream PR \\#: (\\d+)')
        pr_numbers = []

        for sha in shas:
            if self._get_sha_sauce_tag_(sha) != '[nrf fromlist]':
                continue

            body = self._get_sha_body_(sha)
            matches = expr.findall(body)
            if matches:
                pr = int(matches[0])
                if pr not in pr_numbers:
                    pr_numbers.append(pr)

        return pr_numbers

    def _get_unmerged_paths_(self) -> list[str]:
        expr = re.compile('^(U\\D|\\DU) ')
        lines = self._run_cmd_('git', 'status', '--porcelain').splitlines()
        return [line[3:] for line in lines if expr.findall(line)]

    def _find_commit_in_log_(self, commit: dict, commit_log: list[dict]) -> Any:
        for c in commit_log:
            if commit['subject'] != c['subject']:
                continue

            c_files = {(f['file'], f['lines']) for f in c['files']}
            commit_files = {(f['file'], f['lines']) for f in commit['files']}

            if c_files != commit_files:
                continue

            return c

        return None

    def _amend_commit_msg_(self, fmt: str):
        msg = self._run_cmd_('git', 'log', f'--format={fmt}', '-n', '1')
        self._run_cmd_('git', 'commit', '--amend', '-m', msg)

    def _amend_fromtree_commit_msg_(self, commit: dict):
        self._amend_commit_msg_(f'[nrf fromtree] %s %n%n%b%n(cherry picked from commit {commit["sha"]})')

    def _amend_fromlist_commit_msg_(self, commit: dict):
        self._amend_commit_msg_(f'[nrf fromlist] %s %n%n%b%nUpstream PR #: {self.fromlist_pr_map[commit["sha"]]}')

    def _revert_commit_(self, commit: dict) -> tuple[bool, list[str]]:
        applied = True
        unmerged_paths = []

        try:
            self.inf(f'Reverting {self._describe_commit_(commit)}')
            self._run_cmd_('git', 'revert', '-s', '--no-edit', commit['sha'])
        except Exception:
            unmerged_paths = self._get_unmerged_paths_()
            applied = False
            self._run_cmd_('git', 'revert', '--abort')

        return applied, unmerged_paths

    def _cherry_pick_(self, commit: dict) -> tuple[bool, list[str]]:
        applied = True
        unmerged_paths = []

        try:
            self.inf(f'Cherry picking {self._describe_commit_(commit)}')
            self._run_cmd_('git', 'cherry-pick', commit['sha'])
        except Exception:
            unmerged_paths = self._get_unmerged_paths_()
            applied = False
            self._run_cmd_('git', 'cherry-pick', '--abort')

        return applied, unmerged_paths

    def _promote_fromlists_to_fromtrees_(
        self,
        fromlist_log: list[dict],
        fromtree_log: list[dict],
        upstream_log: list[dict]
    ) -> tuple[list[dict], list[dict]]:
        remaining_fromlist_log = []

        for commit in fromlist_log:
            similar_commit = self._find_commit_in_log_(commit, upstream_log)
            if similar_commit is None:
                remaining_fromlist_log.append(commit)
            else:
                fromtree_log.append(similar_commit)

        return remaining_fromlist_log, fromtree_log

    def _filter_already_present_fromtrees_(self, fromtree_log: list[dict], downstream_log: list[dict]) -> list[dict]:
        return [c for c in fromtree_log if self._find_commit_in_log_(c, downstream_log) is None]

    def _update_downstream_tags_(self, downstream_log: list[dict], upstream_log: list[dict]):
        for commit in downstream_log:
            if commit.get('sauce') != 'nrf fromlist':
                continue

            if self._find_commit_in_log_(commit, upstream_log) is not None:
                commit['sauce'] = 'nrf fromtree'
            else:
                commit['sauce'] = 'nrf noup'

            commit.pop('pr-number', None)

    def _sort_commit_log_(self, commit_log: list[dict], reverse: bool=False):
        commit_log.sort(key=lambda c: c['order'], reverse=reverse)

    def _resolve_revert_conflict_(self, unmerged_paths: list[str]):
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
            reapply_commit = self.upstream_log_sha_map[revert_commit['from']]
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

    def _resolve_cherry_pick_conflict_(self, unmerged_paths: list[str]):
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
            c for c in touching_commits if self._find_commit_in_log_(c, self.downstream_log) is None
        ]

        if not touching_commits:
            # No missing commits from upstream, we need to revert a conflicting
            # downstream commit
            self._resolve_revert_conflict_(unmerged_paths)
            return

        self._sort_commit_log_(touching_commits, reverse=True)
        self.fromtree_log.append(touching_commits[0])
        self._sort_commit_log_(self.fromtree_log)
