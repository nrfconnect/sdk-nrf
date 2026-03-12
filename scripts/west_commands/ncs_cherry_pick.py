# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import subprocess
from argparse import Namespace, ArgumentParser
from west.commands import WestCommand
from typing import Any
import re
import copy

class NcsCherryPick(WestCommand):
    def __init__(self) -> None:
        super().__init__(
            name='ncs-cherry-pick',
            help='NCS cherry-pick utility',
            description='Helps cherry-pick commits from OSS projects to a local up-to-date branch',
        )

        self.subject_expr = re.compile('^(\\[[\\w\\s]+\\]) ([\\w\\s\\:\\./]+)$')

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
        )

        parser.add_argument(
            '--project',
            help='OSS project name as defined in the west manifest (zephyr, mcuboot, ...)',
            type=str,
            default='zephyr'
        )

        parser.add_argument(
            '-p',
            '--pristine',
            help='Delete existing branch if any, dropping noup cherry-picks if any',
            action='store_true'
        )

        return parser

    def do_run(self, args: Namespace, _: list[str]) -> None:
        upstream_urls = {
            'zephyr': 'https://github.com/zephyrproject-rtos/zephyr',
            'mcuboot': 'https://github.com/zephyrproject-rtos/mcuboot',
        }

        assert args.project in upstream_urls, f'{args.project} is not supported'

        if args.commits:
            for commit in args.commits:
                assert len(commit) == 40, f'Commit hash {commit} is not valid'

        if args.pr_number:
            assert args.pr_number >= 0 and args.pr_number < 10000000, f'The --pr-number {args.pr_number} is invalid'

        self.args = args
        self.project = self.manifest.get_projects([args.project])[0]

        self.upstream_url = upstream_urls[self.project.name]
        self.downstream_url = self.project.url

        self.fromlist_commits = []
        self.downstream_commits = []
        self.fromtree_commits = []
        self.noup_commits = []
        self.dropped_commits = []
        self.pr_number = None
        self.commit_number_cache = {}

        self.inf(f'Fetching {self.project.name} repository')
        self.upstream_main_commit = self._get_remote_head_commit_(
            self.upstream_url,
            'main'
        )
        self.downstream_main_commit = self._get_remote_head_commit_(
            self.downstream_url,
            'main'
        )
        self.merge_base_commit = self._get_merge_base_(
            self.upstream_main_commit,
            self.downstream_main_commit
        )

        if self.args.pristine or self.args.branch not in self._get_branches_():
            self.inf(f'Creating {self.args.branch} branch')
            self._checkout_ref_(self.downstream_main_commit)
            self._remove_branch_(self.args.branch)
            self._checkout_to_branch_(self.args.branch)
        else:
            self.inf(f'Reading {self.args.branch}')
            self._checkout_ref_(self.args.branch)
            self.fromtree_commits += self._get_forked_fromtree_commits_()
            self.noup_commits += self._get_forked_noup_commits_()
            self.pr_number = self._get_forked_pr_number_()
            self.inf(f'Resetting {self.args.branch}')
            self._reset_to_ref_(self.downstream_main_commit)

        if self.pr_number and self.args.pr_number and self.pr_number != self.args.pr_number:
            self.wrn(f'Changing pull request number from {self.pr_number} to {self.args.pr_number}')

        if self.args.pr_number:
            self.pr_number = self.args.pr_number

        if self.pr_number:
            self.inf(f'Fetching upstream pull request number {self.pr_number}')
            self.fromlist_commits += self._get_pr_commits_(self.upstream_url, self.pr_number)

        if self.args.commits:
            self.fromtree_commits += self.args.commits

        self._promote_fromlists_to_fromtrees_()
        self._sort_fromtrees_()

        if not self.fromlist_commits and not self.fromtree_commits:
            self.inf('No fromlist nor fromtree commits to cherry-pick')
            return

        while True:
            self.inf(f'Resetting branch')
            self._reset_to_ref_(self.downstream_main_commit)

            unmerged_paths = []

            if self.downstream_commits:
                unmerged_paths, commit = self._revert_commits_(self.downstream_commits)

            if unmerged_paths:
                self._resolve_revert_unmerged_paths_(unmerged_paths, commit)
                continue

            unmerged_paths, commit = self._cherry_pick_fromtree_and_fromlist_commits_()

            if not unmerged_paths:
                break

            self._resolve_cherry_pick_unmerged_paths_(unmerged_paths, commit)

        if self.noup_commits:
            self.inf('Cherry-picking the following noup commits:')
            for commit in self.noup_commits:
                self.inf(f'    {self._describe_commit_(commit)}')

            for commit in self.noup_commits:
                if self._cherry_pick_commit_(commit):
                    self.dropped_commits.append(commit)

        if self.dropped_commits:
            self.wrn('The following commits where dropped:')
            for commit in self.dropped_commits:
                self.wrn(f'    {self._describe_commit_(commit)}')

    def _run_cmd_(self, cmd: str, *args) -> str:
        result = subprocess.run(
            [cmd] + list(args),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=self.project.abspath
        )

        assert result.returncode == 0
        return result.stdout.decode().replace('\xa0', ' ')

    def _force_run_cmd_(self, cmd: str, *args):
        subprocess.run(
            [cmd] + list(args),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=self.project.abspath
        )

    def _remove_remote_(self, remote: str):
        self._force_run_cmd_('git', 'remote', 'remove', remote)

    def _add_remote_(self, remote: str, url: str):
        self._run_cmd_('git', 'remote', 'add', remote, url)

    def _fetch_remote_ref_(self, remote: str, ref: str):
        self._run_cmd_('git', 'fetch', remote, ref)

    def _checkout_ref_(self, ref: str):
        self._run_cmd_('git', 'checkout', ref)

    def _reset_to_ref_(self, ref: str):
        self._run_cmd_('git', 'reset', '--hard', ref)

    def _checkout_to_branch_(self, branch: str):
        self._run_cmd_('git', 'checkout', '-b', branch)

    def _get_head_commit_(self, ref: str) -> str:
        return self._run_cmd_('git', 'log', '--format=%H', '-n', '1', ref)[:-1]

    def _get_remote_head_commit_(self, remote: str, ref: str) -> str:
        self._fetch_remote_ref_(remote, ref)
        return self._get_head_commit_('FETCH_HEAD')

    def _get_branches_(self) -> list[str]:
        return self._run_cmd_('git', 'branch', '--format=%(refname:short)').splitlines()

    def _remove_branch_(self, branch: str):
        self._force_run_cmd_('git', 'branch', '-D', branch)

    def _create_branch_(self, branch: str):
        self._run_cmd_('git', 'branch', branch)

    def _get_merge_base_(self, ref_a: str, ref_b: str) -> str:
        return self._run_cmd_('git', 'merge-base', ref_a, ref_b)[:-1]

    def _get_commit_range_(self, from_ref: str, to_ref: str) -> list[str]:
        return self._run_cmd_('git', 'log', '--format=%H', f'{from_ref}..{to_ref}')[:-1].split('\n')

    def _describe_commit_(self, commit: str) -> str:
        return self._run_cmd_('git', 'log', '--format=%h %s', '-n', '1', commit)[:-1]

    def _get_commit_header_(self, commit: str) -> str:
        return self._run_cmd_('git', 'log', '--format=%s', '-n', '1', commit)[:-1]

    def _get_commit_body_(self, commit: str) -> str:
        return self._run_cmd_('git', 'log', '--format=%B', '-n', '1', commit)[:-1]

    def _get_commit_sauce_tag_(self, commit: str) -> str:
        subject = self._get_commit_header_(commit)
        matches = self.subject_expr.findall(subject)
        if matches:
            return matches[0][0]

        return ''

    def _get_pr_commits_(self, remote: str, pr_number: int) -> list[str]:
        pr_head_commit = self._get_remote_head_commit_(remote, f'pull/{pr_number}/head')
        pr_base_commit = self._get_merge_base_(self.upstream_main_commit, pr_head_commit)
        return self._get_commit_range_(pr_base_commit, pr_head_commit)

    def _get_forked_commits_(self) -> list[str]:
        head_commit = self._get_head_commit_('HEAD')

        if self.downstream_main_commit == head_commit:
            return []

        return self._get_commit_range_(self.downstream_main_commit, head_commit)

    def _get_forked_noup_commits_(self) -> list[str]:
        commits = self._get_forked_commits_()
        return [c for c in commits if self._get_commit_sauce_tag_(c) == '[nrf noup]']

    def _get_forked_fromtree_commits_(self) -> list[str]:
        commits = self._get_forked_commits_()
        commits = [c for c in commits if self._get_commit_sauce_tag_(c) == '[nrf fromtree]']
        expr = re.compile('\\(cherry picked from commit (\\w+)\\)')
        return [expr.findall(self._get_commit_body_(c))[0] for c in commits]

    def _get_forked_pr_number_(self) -> Any:
        commits = self._get_forked_commits_()
        for commit in commits:
            if self._get_commit_sauce_tag_(commit) != '[nrf fromlist]':
                continue

            body = self._get_commit_body_(commit)
            expr = re.compile('Upstream PR \\#: (\\d+)')
            return int(expr.findall(body)[0])

        return None

    def _cherry_pick_abort_(self):
            self._run_cmd_('git', 'cherry-pick', '--abort')

    def _get_unmerged_paths_(self) -> list[str]:
        expr = re.compile('^(U\\D|\\DU) ')
        lines = self._run_cmd_('git', 'status', '--porcelain').splitlines()
        return [line[3:] for line in lines if expr.findall(line)]

    def _commit_is_present_(self, commit: str) -> bool:
        present = True

        while True:
            reverting_commits = self._run_cmd_(
                'git',
                'log',
                f'--grep=This reverts commit {commit}',
                '--format=%H',
                f'{self.merge_base_commit}..HEAD'
            ).splitlines()

            if not reverting_commits:
                break

            assert len(reverting_commits) == 1
            present = not present
            commit = reverting_commits[0]

        return present

    def _get_diff_summary_(self, commit: str) -> str:
        return self._run_cmd_('git', 'show', '--format=%n', '-n', '1', '--summary', commit)[:-1]

    def _find_similar_commit_(self, commit: str) -> Any:
        subject = self._run_cmd_('git', 'log', '--format=%s', '-n', '1', commit)[:-1]
        matches = self.subject_expr.findall(subject)
        subject = matches[0][1] if matches else subject
        patch = self._get_diff_summary_(commit)
        similar_commits = self._run_cmd_(
            'git',
            'log',
            f'--grep={subject}',
            '--format=%H',
            f'{self.merge_base_commit}..HEAD'
        ).splitlines()

        for similar_commit in similar_commits:
            similar_patch = self._get_diff_summary_(similar_commit)
            if patch == similar_patch:
                return similar_commit

        return None

    def _find_similar_present_commit_(self, commit: str) -> Any:
        similar_commit = self._find_similar_commit_(commit)

        if similar_commit is not None and self._commit_is_present_(similar_commit):
            return similar_commit

        return None

    def _commit_is_empty_(self, commit: str) -> bool:
        try:
            self._run_cmd_('git', 'cherry-pick', commit)
        except:
            unmerged_paths = self._get_unmerged_paths_()
            self._cherry_pick_abort_()
            return False if unmerged_paths else True

        self._reset_to_ref_('HEAD~1')
        return False

    def _amend_commit_msg_(self, fmt: str):
        msg = self._run_cmd_('git', 'log', f'--format={fmt}', '-n', '1')
        self._run_cmd_('git', 'commit', '--amend', '-m', msg)

    def _amend_fromtree_commit_msg_(self, commit: str):
        self._amend_commit_msg_(f'[nrf fromtree] %s %n%n%b%n(cherry picked from commit {commit})')

    def _amend_fromlist_commit_msg_(self, pr_number: int):
         self._amend_commit_msg_(f'[nrf fromlist] %s %n%n%b%nUpstream PR #: {pr_number}')

    def _revert_commit_(self, commit: str) -> list[str]:
        unmerged_paths = []

        try:
            self.inf(f'Reverting {self._describe_commit_(commit)}')
            self._run_cmd_('git', 'revert', '-s', '--no-edit', commit)
        except:
            unmerged_paths = self._get_unmerged_paths_()
            assert unmerged_paths, f'Revert of {self._describe_commit_(commit)} is empty'
            self.dbg(f'The following files are conflicting')
            for unmerged_path in unmerged_paths:
                self.dbg(f'    {unmerged_path}')
            self._run_cmd_('git', 'revert', '--abort')

        return unmerged_paths

    def _cherry_pick_commit_(self, commit: str):
        unmerged_paths = []

        try:
            self.inf(f'Cherry picking {self._describe_commit_(commit)}')
            self._run_cmd_('git', 'cherry-pick', commit)
        except:
            unmerged_paths = self._get_unmerged_paths_()
            assert unmerged_paths, f'Cherry pick of {self._describe_commit_(commit)} is empty'
            self.dbg(f'The following files are conflicting')
            for unmerged_path in unmerged_paths:
                self.dbg(f'    {unmerged_path}')
            self._cherry_pick_abort_()

        return unmerged_paths

    def _cherry_pick_fromtree_commits_(self) -> tuple[list[str], str]:
        unmerged_paths = []

        for commit in reversed(self.fromtree_commits):
            unmerged_paths = self._cherry_pick_commit_(commit)

            if unmerged_paths:
                return unmerged_paths, commit

            self._amend_fromtree_commit_msg_(commit)

        return unmerged_paths, None

    def _cherry_pick_fromlist_commits_(self) -> tuple[list[str], str]:
        unmerged_paths = []

        for commit in reversed(self.fromlist_commits):
            unmerged_paths = self._cherry_pick_commit_(commit)

            if unmerged_paths:
                return unmerged_paths, commit

            self._amend_fromlist_commit_msg_(self.pr_number)

        return unmerged_paths, None

    def _revert_commits_(self, commits: list[str]) -> tuple[list[str], str]:
        unmerged_paths = []

        for commit in commits:
            unmerged_paths = self._revert_commit_(commit)

            if unmerged_paths:
                return unmerged_paths, commit

        return unmerged_paths, None

    def _cherry_pick_fromtree_and_fromlist_commits_(self) -> tuple[list[str], str]:
        unmerged_paths, picked_commit = self._cherry_pick_fromtree_commits_()

        if unmerged_paths:
            return unmerged_paths, picked_commit

        return self._cherry_pick_fromlist_commits_()

    def _get_path_history_(self, path: str, commit: str) -> list[str]:
        def follow_path(path: str) -> list[str]:
            return self._run_cmd_(
                'git',
                'log',
                '--follow',
                '--format=%H',
                f'{self.merge_base_commit}..HEAD',
                '--',
                path
            ).splitlines()

        def get_path_rename(commit: str, path: str) -> Any:
            expr = re.compile(f'^R\\d\\d\\d\\s+([\\w/\\.]+)\\s+([\\w/\\.]+)$', re.MULTILINE)
            text = self._run_cmd_(
                'git',
                'show',
                '--name-status',
                commit
            )

            if text == '':
                return None

            matches = expr.findall(text)

            for m in matches:
                if m[0] == path:
                    return m[1]

            return None

        path_rename = get_path_rename(commit, path)
        path = path_rename if path_rename is not None else path
        return follow_path(path)

    def _get_commit_number_(self, commit: str) -> int:
        if commit in self.commit_number_cache:
            return self.commit_number_cache[commit]

        commit_number = int(self._run_cmd_('git', 'rev-list', '--count', commit)[:-1])
        self.commit_number_cache[commit] = commit_number
        return commit_number

    def _sort_commits_(self, commits: list[str]):
        commits = list(set(commits))
        commits = [[commit, self._get_commit_number_(commit)] for commit in commits]
        commits = sorted(commits, key=lambda commit: commit[1], reverse=True)
        return [commit[0] for commit in commits]

    def _get_reverted_commit_(self, commit: str) -> Any:
        expr = re.compile('^This reverts commit (\\w{40})\\.$', re.MULTILINE)
        body = self._get_commit_body_(commit)
        matches = expr.findall(body)
        if not matches:
            return None
        return matches[0]

    def _filter_redundant_commits_(self, commits: list[str]) -> list[str]:
        keep_commits = []
        reverted_commits = []

        for commit in commits:
            if commit in reverted_commits:
                continue

            reverted_commit = self._get_reverted_commit_(commit)

            if reverted_commit is None:
                keep_commits.append(commit)
                continue

            reverted_commits.append(reverted_commit)

        return keep_commits

    def _get_all_modifying_commits_(self, unmerged_paths: list[str], commit: str) -> list[str]:
        commits = []

        for path in unmerged_paths:
            commits += self._get_path_history_(path, commit)

        return self._sort_commits_(commits)

    def _filter_synced_commits(self, downstream_commits: list[str], upstream_commits: list[str]) -> tuple[list[str], list[str]]:
        desynced_downstream_commits = downstream_commits
        desynced_upstream_commits = []

        self._checkout_ref_(self.downstream_main_commit)
        for commit in upstream_commits:
            similar_commit = self._find_similar_present_commit_(commit)
            if similar_commit is None:
                desynced_upstream_commits.append(commit)
            else:
                desynced_downstream_commits.remove(similar_commit)

        self._checkout_ref_(self.upstream_main_commit)

        return desynced_downstream_commits, desynced_upstream_commits

    def _resolve_cherry_pick_unmerged_paths_(self, unmerged_paths: list[str], picked_commit: str):
        # Get all commits modifying the file present in downstream since downstream forked.
        self._checkout_ref_(self.downstream_main_commit)
        downstream_modifications = self._get_all_modifying_commits_(unmerged_paths, picked_commit)
        downstream_modifications = self._filter_redundant_commits_(downstream_modifications)

        # Get all commits modifying the file present in upstream since downstream forked.
        self._checkout_ref_(self.upstream_main_commit)
        upstream_modifications = self._get_all_modifying_commits_(unmerged_paths, picked_commit)
        upstream_modifications = self._filter_redundant_commits_(upstream_modifications)

        downstream_modifications, upstream_modifications = self._filter_synced_commits(
            downstream_modifications,
            upstream_modifications
        )

        #if downstream_modifications and upstream_modifications:
        #    for i, commit in enumerate(reversed(copy.copy(downstream_modifications))):
        #        if self._find_similar_commit_(commit) != upstream_modifications[-i]:
        #            break

        #        downstream_modifications = downstream_modifications[:-1]
        #        upstream_modifications = upstream_modifications[:-1]

        # The remaining commits, if any, are actionable. We need to revert the desynced
        # downstream commits, and cherry-pick the desynced/missing upstream commits.
        assert downstream_modifications or upstream_modifications

        fromtree_commits_len = len(self.fromtree_commits)

        appended_commit = None

        for commit in reversed(upstream_modifications):
            if commit not in self.fromtree_commits:
                self.fromtree_commits.append(commit)
                self.dbg(f'Cherry pick {self._describe_commit_(commit)}')
                self.fromtree_commits = self._sort_commits_(self.fromtree_commits)
                appended_commit = commit
                break

        if appended_commit is not None:
            self._checkout_ref_(self.args.branch)
            return

        assert downstream_modifications

        for commit in downstream_modifications:
            if commit not in self.downstream_commits:
                self.downstream_commits.append(commit)
                self.downstream_commits = self._sort_commits_(self.downstream_commits)
                appended_commit = commit
                break

        assert appended_commit is not None

        similar_commit = self._find_similar_present_commit_(appended_commit)
        if similar_commit is None:
            self.dbg(f'Drop {self._describe_commit_(appended_commit)}')
            self.dropped_commits.append(appended_commit)
            self.dropped_commits = self._sort_commits_(self.dropped_commits)
        else:
            self.dbg(f'Reapply {self._describe_commit_(similar_commit)}')
            self.fromtree_commits.append(similar_commit)
            self.fromtree_commits = self._sort_commits_(self.fromtree_commits)

        self._checkout_ref_(self.args.branch)

    def _resolve_revert_unmerged_paths_(self, unmerged_paths: list[str], reverted_commit: str):
        # Get all commits modifying the file present in downstream since downstream forked.
        self._checkout_ref_(self.downstream_main_commit)
        downstream_modifications = self._get_all_modifying_commits_(unmerged_paths, reverted_commit)
        downstream_modifications = self._filter_redundant_commits_(downstream_modifications)

        # Get all commits modifying the file present in upstream since downstream forked.
        self._checkout_ref_(self.upstream_main_commit)
        upstream_modifications = self._get_all_modifying_commits_(unmerged_paths, reverted_commit)
        upstream_modifications = self._filter_redundant_commits_(upstream_modifications)

        downstream_modifications, upstream_modifications = self._filter_synced_commits(
            downstream_modifications,
            upstream_modifications
        )

        # Remove commits which are already synced between upstream and downstream since
        # reverting these to then reapply them is redundant. Downstream these are fromlist
        # or fromtrees, so we need to look for matching commits in upstream.
        # We know the merge base is synced, so we look from the merge base towards main.
        #if downstream_modifications and upstream_modifications:

        #    for i, commit in enumerate(reversed(copy.copy(downstream_modifications))):
        #        if self._find_similar_commit_(commit) != upstream_modifications[-i]:
        #            break

        #        downstream_modifications = downstream_modifications[:-1]
        #        upstream_modifications = upstream_modifications[:-1]

        # The remaining commits, if any, are actionable. We need to revert the desynced
        # downstream commits, and cherry-pick the desynced/missing upstream commits.
        assert downstream_modifications or upstream_modifications

        appended_commit = None

        # Sorting takes time so only sort if commits are added
        if downstream_modifications:
            for commit in downstream_modifications:
                if commit not in self.downstream_commits:
                    self.downstream_commits.append(commit)
                    self.downstream_commits = self._sort_commits_(self.downstream_commits)
                    appended_commit = commit
                    break

        # If we failed to revert a commit, the only fix is to revert a newer commit.
        assert appended_commit is not None, 'Failed to find downstream commit to revert'

        # We need to track any commit we revert from downstream. If there is a matching
        # commit upstream we simply add it to the fromtree commits to be cherry picked
        # back. If the commit is a conflicting noup or fromlist, we add it to a list of
        # commits for the user to manually reapply or drop.
        similar_commit = self._find_similar_commit_(appended_commit)
        if similar_commit is None:
            self.dbg(f'Drop {self._describe_commit_(appended_commit)}')
            self.dropped_commits.append(appended_commit)
            self.dropped_commits = self._sort_commits_(self.dropped_commits)
        else:
            self.dbg(f'Reapply {self._describe_commit_(similar_commit)}')
            self.fromtree_commits.append(similar_commit)
            self.fromtree_commits = self._sort_commits_(self.fromtree_commits)

        self._checkout_ref_(self.args.branch)

    def _promote_fromlists_to_fromtrees_(self):
        fromlist_commits = []
        fromtree_commits = []

        self._checkout_ref_(self.upstream_main_commit)
        for commit in self.fromlist_commits:
            similar_commit = self._find_similar_commit_(commit)
            if similar_commit is not None:
                fromtree_commits.append(similar_commit)
            else:
                fromlist_commits.append(commit)

        self.fromlist_commits = fromlist_commits
        self.fromtree_commits += fromtree_commits
        self._checkout_ref_(self.args.branch)

    def _sort_fromtrees_(self):
        upstream_commits = []
        self._checkout_ref_(self.upstream_main_commit)
        for commit in self.fromtree_commits:
            similar_commit = self._find_similar_commit_(commit)
            assert similar_commit is not None, f'Failed to find {self._describe_commit_(commit)} in upstream'
            upstream_commits.append(similar_commit)

        missing_upstream_commits = []
        self._checkout_ref_(self.downstream_main_commit)
        for commit in upstream_commits:
            similar_commit = self._find_similar_commit_(commit)

            if similar_commit is not None and self._commit_is_present_(similar_commit):
                continue

            missing_upstream_commits.append(commit)

        self._checkout_ref_(self.args.branch)
        self.fromtree_commits = self._sort_commits_(missing_upstream_commits)
