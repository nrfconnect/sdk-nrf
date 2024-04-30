# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''The "ncs-xyz" extension commands.'''

from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess
import sys
from textwrap import dedent
from typing import Any, List, NamedTuple, Optional, TYPE_CHECKING

from west import log
from west.commands import WestCommand
from west.manifest import Manifest, MalformedManifest, ImportFlag, \
    MANIFEST_PROJECT_INDEX, Project, QUAL_MANIFEST_REV_BRANCH
from west.version import __version__ as west_version
import yaml

try:
    from packaging import version
    import pygit2  # type: ignore
except ImportError:
    sys.exit("Can't import extra dependencies needed for NCS west extensions.\n"
             "Please install packages in nrf/scripts/requirements-extra.txt "
             "with pip3.")

import ncs_west_helpers as nwh
from pygit2_helpers import commit_affects_files, commit_title, \
    title_has_sauce, title_is_revert

WEST_V0_13_0_OR_LATER = version.parse(west_version) >= version.parse('0.13.0')

class NcsUserData(NamedTuple):
    '''Represents userdata in a project in ncs/nrf/west.yml.'''

    upstream_url: str
    upstream_sha: str
    compare_by_default: bool

def ncs_userdata(project: Project) -> Optional[NcsUserData]:
    # Converts raw userdata in 'project' to an NcsUserData, if
    # possible. Otherwise returns None.

    raw_userdata = project.userdata

    if not (raw_userdata and
            isinstance(raw_userdata, dict) and
            'ncs' in raw_userdata):
        return None

    ncs_dict = raw_userdata['ncs']

    return NcsUserData(upstream_url=ncs_dict['upstream-url'],
                       upstream_sha=ncs_dict['upstream-sha'],
                       compare_by_default=ncs_dict['compare-by-default'])

def add_zephyr_rev_arg(parser: argparse.ArgumentParser) -> None:
    parser.add_argument('-z', '--zephyr-rev', metavar='REF',
                        help='''zephyr git ref (commit, branch, etc.);
                        default: upstream/main''')

def add_projects_arg(parser: argparse.ArgumentParser) -> None:
    parser.add_argument('projects', metavar='PROJECT', nargs='*',
                        help='projects (by name or path) to operate on')

def repo_analyzer(ncs_project: Project,
                  ncs_sha: str,
                  upstream_project: Project,
                  upstream_sha: str) -> nwh.RepoAnalyzer:
    upstream_repository = to_repository(upstream_project, upstream_sha)
    upstream_repository.name = to_ncs_name(upstream_project)

    return nwh.RepoAnalyzer(to_repository(ncs_project, ncs_sha),
                            upstream_repository)

def to_repository(project: Project, sha: str) -> nwh.Repository:
    # Converts west.manifest.Project to the more abstract format
    # expected by ncs_west_helpers.

    assert project.abspath
    return nwh.Repository(name=project.name,
                          path=project.path,
                          abspath=project.abspath,
                          url=project.url,
                          sha=sha)

def print_likely_merged(downstream_repo: nwh.Repository,
                        upstream_repo: nwh.Repository) -> None:
    analyzer = nwh.RepoAnalyzer(downstream_repo, upstream_repo)
    likely_merged = analyzer.likely_merged
    if likely_merged:
        # likely_merged is a map from downstream commits to
        # lists of upstream commits that look similar.
        log.msg('downstream patches which are likely merged upstream',
                '(revert these if appropriate):', color=log.WRN_COLOR)
        for downstream_commit, upstream_commits in likely_merged.items():
            if len(upstream_commits) == 1:
                log.inf(f'- {downstream_commit.oid} {commit_title(downstream_commit)}')
                log.inf(f'  Similar upstream title:\n'
                        f'  {upstream_commits[0].oid} {commit_title(upstream_commits[0])}')
            else:
                log.inf(f'- {downstream_commit.oid} {commit_title(downstream_commit)}\n'
                        '  Similar upstream titles:')
                for i, upstream_commit in enumerate(upstream_commits, start=1):
                    log.inf(f'    {i}. {upstream_commit.oid} {commit_title(upstream_commit)}')
    else:
        log.dbg('no downstream patches seem to have been merged upstream')

def to_ncs_name(zp: Project) -> str:
    # convert zp, a west.manifest.Project in the zephyr manifest,
    # to the equivalent Project name in the NCS manifest.
    #
    # TODO: does the fact that this is needed mean the west commit
    #       which names the manifest project 'manifest' unconditionally
    #       was wrong to do so?
    if zp.name == 'manifest':
        return 'zephyr'
    else:
        return zp.name

class NcsWestCommand(WestCommand):
    # some common code that will be useful to multiple commands

    @staticmethod
    def checked_sha(project: Project, revision: str) -> Optional[str]:
        # get revision's SHA and check that it exists in the project.
        # returns the SHA on success, or None if we can't unwrap revision
        # to a SHA which is present in the project.

        try:
            sha = project.sha(revision)
            project.git('cat-file -e ' + sha)
            return sha
        except (subprocess.CalledProcessError, FileNotFoundError):
            return None

    def setup_upstream_downstream(self, args: argparse.Namespace) -> None:
        # set some instance state that will be useful for
        # comparing upstream and downstream.
        #
        # below, "pmap" means "project map": a dict mapping project
        # names to west.manifest.Project instances.

        # zephyr_rev: validated --zephyr-rev argument. (quits and
        #             print a message about what to do if things go
        #             wrong).
        # z_manifest: upstream zephyr west.manifest.Manifest instance
        # z_pmap: project map for the zephyr manifest
        self.zephyr_rev = self.validate_zephyr_rev(args)
        self.z_manifest = self.zephyr_manifest()
        self.z_pmap = {p.name: p for p in self.z_manifest.projects}

        # we want to build an ncs_pmap too. if we have a projects arg,
        # we'll use that. otherwise, we'll just use all the projects
        # in the NCS west manifest.
        if hasattr(args, 'projects') and args.projects:
            try:
                projects = self.manifest.get_projects(
                    args.projects, only_cloned=True)
            except ValueError as ve:
                # West guarantees that get_projects()'s ValueError
                # has exactly two values in args.
                unknown, uncloned = ve.args
                if unknown:
                    log.die('unknown projects:', ', '.join(unknown))
                self.die_if_any_uncloned(uncloned)
        else:
            projects = [project for project in self.manifest.projects
                        if self.to_upstream_repository(project) is not None]
            self.die_if_any_uncloned(projects)
        self.ncs_pmap = {p.name: p for p in projects}

    @staticmethod
    def die_if_any_uncloned(projects: List[Project]) -> None:
        uncloned = [project for project in projects if not project.is_cloned()]
        if not uncloned:
            return
        log.die('uncloned downstream projects:',
                ', '.join(project.name for project in uncloned) + '\n' +
                'Run "west update", then retry.')

    def zephyr_manifest(self) -> Manifest:
        # Load the upstream manifest. Since west v0.13, the resulting
        # projects have no absolute paths, so we'll fix those up so
        # they're relative to our own topdir. (We can't use
        # Manifest.from_file() in this case because self.zephyr_rev is
        # not what's checked out on the file system).

        z_project = self.manifest.get_projects(['zephyr'],
                                               allow_paths=False,
                                               only_cloned=True)[0]
        cp = z_project.git(f'show {self.zephyr_rev}:west.yml',
                           capture_stdout=True, check=True)
        z_west_yml = cp.stdout.decode('utf-8')
        try:
            ret = Manifest.from_data(source_data=yaml.safe_load(z_west_yml),
                                     import_flags=ImportFlag.IGNORE)
            if WEST_V0_13_0_OR_LATER:
                for project in ret.projects:
                    project.topdir = self.manifest.topdir
            return ret
        except MalformedManifest:
            log.die(f"can't load zephyr manifest; file {z_west_yml} "
                    "is malformed")

    def validate_zephyr_rev(self, args: argparse.Namespace) -> str:
        if args.zephyr_rev:
            zephyr_rev = args.zephyr_rev
        else:
            zephyr_rev = 'upstream/main'
        zephyr_project = self.manifest.get_projects(['zephyr'])[0]
        try:
            self.zephyr_sha = zephyr_project.sha(zephyr_rev)
        except subprocess.CalledProcessError:
            msg = f'unknown ZEPHYR_REV: {zephyr_rev}'
            if not args.zephyr_rev:
                msg += ('\nPlease run this in the zephyr repository and retry '
                        '(or use --zephyr-rev):' +
                        f'\n  git remote add -f upstream {_UPSTREAM_ZEPHYR_URL}')
            self.parser.error(msg)
        return zephyr_rev

    def to_ncs_repository(self, project: Project) -> Optional[nwh.Repository]:
        # Get necessary NCS-side metadata for the project and convert to
        # a Repository.

        ncs_rev = 'refs/heads/manifest-rev'
        ncs_sha = self.checked_sha(project, ncs_rev)
        if ncs_sha is None:
            if not self.manifest.is_active(project):
                help_url = 'https://docs.zephyrproject.org/latest/develop/west/manifest.html#project-groups-and-active-projects'
                log.err(f"{project.name_and_path}: can't get loot: "
                        "project is not cloned and is not active. "
                        'To get loot, first enable these groups: '
                        f"{' '.join(project.groups)} (see {help_url}). "
                        'Then, run "west update".')
            else:
                log.err(f"{project.name_and_path}: can't get loot: "
                        'project is not cloned; please run "west update"')
            return None
        return to_repository(project, ncs_sha)

    def to_upstream_repository(self, project: Project) -> Optional[nwh.Repository]:
        # Get the upstream metadata for the project and convert to a
        # Repository.
        name = project.name
        userdata = ncs_userdata(project)

        if userdata:
            upstream_url = userdata.upstream_url
            upstream_rev = userdata.upstream_sha
        elif name in self.z_pmap and name != 'manifest':
            upstream_url = self.z_pmap[name].url
            upstream_rev = self.z_pmap[name].revision
        elif name == 'zephyr':
            upstream_url = _UPSTREAM_ZEPHYR_URL
            upstream_rev = self.zephyr_rev
        else:
            return None

        upstream_sha = self.checked_sha(project, upstream_rev)
        if upstream_sha is None:
            log.wrn(f"{project.name_and_path}: can't get loot: "
                    "please fetch upstream URL "
                    f'{upstream_url} (need revision {upstream_rev})')
            return None
        if TYPE_CHECKING:
            assert project.abspath
        return nwh.Repository(name=project.name,
                              path=project.path,
                              abspath=project.abspath,
                              url=upstream_url,
                              sha=upstream_sha)

class NcsLoot(NcsWestCommand):

    def __init__(self):
        super().__init__(
            'ncs-loot',
            'list out of tree (downstream only) NCS patches',
            dedent('''
            List commits which are not present in upstream repositories.

            This command lists patches which are added for the NCS
            in a downstream repository that have not been reverted.

            Downstream revisions are loaded from nrf/west.yml.'''))

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description)
        add_zephyr_rev_arg(parser)
        parser.add_argument('-s', '--sha', dest='sha_only',
                            action='store_true',
                            help='only print SHAs of OOT commits')
        parser.add_argument('-j', '--json', type=Path,
                            help='''save per-project info in the given
                            JSON file; the format is internal and may
                            change''')
        parser.add_argument('-f', '--file', dest='files', metavar='FILE',
                            action='append',
                            help='''list patches changing this file;
                            may be given multiple times''')
        add_projects_arg(parser)
        return parser

    def do_run(self, args: argparse.Namespace, unknown_args: list[str]):
        if args.files and len(args.projects) != 1:
            self.parser.error('--file used, so expected 1 project argument, '
                              f'but got: {len(args.projects)}')
        self.setup_upstream_downstream(args)
        self.args = args
        self.json_data: dict[str, Any] = {}

        for name, project in self.ncs_pmap.items():
            if name == 'manifest':
                continue

            ncs_repository = self.to_ncs_repository(project)
            if not ncs_repository:
                continue

            upstream_repository = self.to_upstream_repository(project)
            if not upstream_repository:
                continue

            loot = self.get_loot(ncs_repository, upstream_repository)
            self.print_loot(loot, ncs_repository, upstream_repository)

        if args.json:
            with open(args.json, 'w') as f:
                json.dump(self.json_data, f)

    @staticmethod
    def get_loot(ncs_repository: nwh.Repository,
                 upstream_repository: nwh.Repository) -> list[pygit2.Commit]:
        # Create analyzer object and get the loot for a pair of
        # upstream/downstream repositories.
        name_path = f'{ncs_repository.name} ({ncs_repository.path})'

        try:
            analyzer = nwh.RepoAnalyzer(ncs_repository, upstream_repository)
        except nwh.InvalidRepositoryError as ire:
            log.die(f"{name_path}: {str(ire)}")

        try:
            return analyzer.downstream_outstanding
        except nwh.UnknownCommitsError as uce:
            log.die(f'{name_path}: unknown commits: {str(uce)}')

    def print_loot(self,
                   loot: list[pygit2.Commit],
                   ncs_repository: nwh.Repository,
                   upstream_repository: nwh.Repository) -> None:
        # Print a list of out of tree outstanding patches.
        #
        # loot: the list itself
        # ncs_repository: contains NCS information
        # upstream_repository: contains upstream information

        if not loot and log.VERBOSE <= log.VERBOSE_NONE:
            # Don't print output if there's no loot unless verbose
            # mode is on.
            return

        log.banner(f'{ncs_repository.name} ({ncs_repository.path})')
        log.inf(f'     NCS commit: {ncs_repository.sha}\n'
                f'   upstream URL: {upstream_repository.url}\n'
                f'upstream commit: {upstream_repository.sha}')
        log.inf(f'    OOT patches: {len(loot)}' +
                (', output limited by --file' if self.args.files else ''))

        json_sha_list = []
        json_title_list = []
        for index, commit in enumerate(loot):
            if self.args.files and not commit_affects_files(commit,
                                                            self.args.files):
                log.dbg(f"skipping {commit.oid}; it doesn't affect file filter",
                        level=log.VERBOSE_VERY)
                continue

            sha = str(commit.oid)
            title = commit_title(commit)
            if self.args.sha_only:
                log.inf(sha)
            else:
                # Emit a warning if we have a non-revert patch with an
                # incorrect sauce tag. (Again, downstream might carry
                # reverts of upstream patches, which we shouldn't warn
                # about.)
                if not title_has_sauce(title) and not title_is_revert(title):
                    space = ' ' * len(f'{index+1}. ')
                    sauce_note = \
                        f'\n{space}[NOTE: commit title is missing a sauce tag]'
                else:
                    sauce_note = ''
                log.inf(f'{index+1}. {sha} {title}{sauce_note}')

            if self.args.json:
                json_sha_list.append(sha)
                json_title_list.append(title)

        if self.args.json:
            self.json_data[ncs_repository.name] = {
                'path': ncs_repository.path,
                'ncs-commit': ncs_repository.sha,
                'upstream-commit': upstream_repository.sha,
                'shas': json_sha_list,
                'titles': json_title_list,
            }

class NcsCompare(NcsWestCommand):
    def __init__(self):
        super().__init__(
            'ncs-compare',
            'compare upstream manifest with NCS',
            dedent('''
            Compares project status between zephyr/west.yml and your
            current NCS west manifest-rev branches (i.e. the results of your
            most recent 'west update').

            Use --verbose to include information on blocked
            upstream projects.'''))

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description)
        add_zephyr_rev_arg(parser)
        parser.add_argument('--all', action='store_true',
                            help='''compare all projects possible, including
                            those with 'compare-by-default: false' in their
                            NCS userdata''')
        return parser

    def do_run(self, args: argparse.Namespace,
               unknown_args: list[str]) -> None:
        self.setup_upstream_downstream(args)

        # Get a dict containing projects that are in the NCS which are
        # *not* imported from Zephyr in nrf/west.yml. We will treat
        # these specially to make the output easier to understand.
        ncs_only = Manifest.from_file(import_flags=ImportFlag.IGNORE_PROJECTS)
        ncs_only_projects = ncs_only.projects[MANIFEST_PROJECT_INDEX + 1:]
        ncs_only_names = set(p.name for p in ncs_only_projects)
        # This is a dict mapping names of projects which *are* imported
        # from zephyr to the Project instances.
        self.imported_pmap = {name: project
                              for name, project in self.ncs_pmap.items()
                              if name not in ncs_only_names}

        log.inf('Comparing your manifest-rev branches with zephyr/west.yml '
                f'at {self.zephyr_rev}' +
                (', sha: ' + self.zephyr_sha
                 if self.zephyr_rev != self.zephyr_sha else ''))
        log.inf()

        present_blocked = []
        present_allowed = []
        missing_blocked = []
        missing_allowed = []
        for zephyr_project in self.z_pmap.values():
            ncs_name = to_ncs_name(zephyr_project)
            present = ncs_name in self.ncs_pmap
            blocked = Path(zephyr_project.path) in _BLOCKED_PROJECTS
            if present:
                if blocked:
                    present_blocked.append(zephyr_project)
                else:
                    present_allowed.append(zephyr_project)
            else:
                if blocked:
                    missing_blocked.append(zephyr_project)
                else:
                    missing_allowed.append(zephyr_project)

        def print_lst(projects):
            for project in projects:
                log.inf(f'{project.name_and_path}')

        if missing_blocked and log.VERBOSE >= log.VERBOSE_NORMAL:
            log.banner('blocked zephyr projects',
                       'not in nrf (these are all OK):')
            print_lst(missing_blocked)

        log.banner('blocked zephyr projects in NCS:')
        if present_blocked:
            log.wrn(f'these should all be removed from {self.manifest.path}!')
            print_lst(present_blocked)
        else:
            log.inf('none (OK)')

        log.banner('non-blocked zephyr projects missing from NCS:')
        if missing_allowed:
            west_yml = self.manifest.path
            log.wrn(
                f'missing projects should be added to NCS or blocked\n'
                f"  To add to NCS:\n"
                f"    1. do the zephyr mergeup\n"
                f"    2. update zephyr revision in {west_yml}\n"
                f"    3. add projects to zephyr's name-allowlist in "
                f"{west_yml}\n"
                f"    4. run west {self.name} again to check your work\n"
                f"  To block: edit _BLOCKED_PROJECTS in {__file__}")
            for project in missing_allowed:
                log.small_banner(f'{project.name_and_path}:')
                log.inf(f'upstream revision: {project.revision}')
                log.inf(f'upstream URL: {project.url}')
        else:
            log.inf('none (OK)')

        if present_allowed:
            log.banner('projects in both zephyr and NCS:')
            for zephyr_project in present_allowed:
                ncs_project = self.ncs_pmap[to_ncs_name(zephyr_project)]
                if ncs_project.userdata is not None:
                    log.small_banner(f'{ncs_project.name_and_path}:')
                    log.inf('This project has a custom upstream specified via '
                            'userdata in nrf/west.yml; see below output for '
                            'details')
                    continue

                if ncs_project.name == 'zephyr':
                    upstream_url = _UPSTREAM_ZEPHYR_URL
                    upstream_revision = self.zephyr_rev
                else:
                    upstream_url = zephyr_project.url
                    upstream_revision = zephyr_project.revision

                self.compare_project_with_upstream(ncs_project,
                                                   upstream_url,
                                                   upstream_revision)

        log.banner('NCS projects with upstreams specified via userdata:')
        projects_with_userdata = [project for project in ncs_only_projects
                                  if project.userdata]
        any_needs_update = False
        for ncs_project in projects_with_userdata:
            userdata = ncs_userdata(ncs_project)
            assert userdata
            if args.all or userdata.compare_by_default:
                needs_update = self.compare_project_with_upstream(
                    ncs_project,
                    userdata.upstream_url,
                    userdata.upstream_sha)
                if needs_update:
                    any_needs_update = True
        if not any_needs_update:
            log.inf('Everything is up to date or ahead of its upstream-sha.')

        if log.VERBOSE <= log.VERBOSE_NONE:
            log.inf('\nNote: verbose output was omitted,',
                    'use "west -v ncs-compare" for more details.')

    def compare_project_with_upstream(
            self,
            ncs_project: Project,
            upstream_url: str,
            upstream_revision: str
    ) -> bool:
        '''Compare the NCS version of a project with its upstream
        counterpart.

        Returns True if the project comparison means it needs, or may
        need, an update. I.e., returns True if the project is behind
        its upstream counterpart or has diverged from it, or if the
        comparison failed due to missing data.
        '''

        # is_imported is true if we imported this project from the
        # zephyr manifest rather than defining it directly ourselves
        # in nrf/west.yml. This is important because imported projects
        # are basically always updated by upmerging zephyr itself and
        # automatically getting the new revision in the updated
        # zephyr/west.yml.
        is_imported = ncs_project.name in self.imported_pmap
        imported = ', imported from zephyr' if is_imported else ''
        banner = f'{ncs_project.name} ({ncs_project.path}){imported}:'
        ncs_sha = self.checked_sha(ncs_project, QUAL_MANIFEST_REV_BRANCH)
        upstream_sha = self.checked_sha(ncs_project, upstream_revision)

        if not ncs_project.is_cloned() or ncs_sha is None or upstream_sha is None:
            log.small_banner(banner)
            if not ncs_project.is_cloned():
                log.wrn('project is not cloned; please run "west update"')
            elif ncs_sha is None:
                log.wrn(f"can't compare; please run \"west update {ncs_project.name}\" "
                        f'(need revision {ncs_project.revision})')
            elif upstream_sha is None:
                log.wrn(f"can't compare; please fetch upstream URL {upstream_url} "
                        f'(need revision {upstream_revision})')
            return True

        patch_counts = ncs_project.git(
            f'rev-list --left-right --count {upstream_sha}...{ncs_sha}',
            capture_stdout=True).stdout.split()
        behind, ahead = [int(c) for c in patch_counts]

        if upstream_sha == ncs_sha:
            status = 'up to date'
        elif ahead and not behind:
            status = f'ahead by {ahead} commit' + ("s" if ahead > 1 else "")
        elif ncs_project.is_ancestor_of(ncs_sha, upstream_sha):
            status = f'behind by {behind} commit' + ("s" if behind > 1 else "")
        else:
            status = f'diverged: {ahead} ahead, {behind} behind'
        up_or_ahead = 'up to date' in status or 'ahead by' in status

        commits = (f'     NCS commit: {ncs_sha}\n'
                   f'   upstream URL: {upstream_url}\n'
                   f'upstream commit: {upstream_sha}')
        ncs_repo = to_repository(ncs_project, ncs_sha)
        assert ncs_project.abspath
        upstream_repo = nwh.Repository(name=ncs_project.name,
                                       path=ncs_project.path,
                                       abspath=ncs_project.abspath,
                                       url=upstream_url,
                                       sha=upstream_sha)
        if up_or_ahead:
            if log.VERBOSE > log.VERBOSE_NONE:
                # Up to date or ahead: only print in verbose mode.
                log.small_banner(banner)
                log.inf(commits)
                log.inf(status)
                print_likely_merged(ncs_repo, upstream_repo)
        else:
            # Behind or diverged: always print.
            if is_imported and 'behind by' in status:
                status += ' and imported: update by doing zephyr mergeup'

            log.small_banner(banner)
            log.inf(commits)
            log.msg(status, color=log.WRN_COLOR)
            print_likely_merged(ncs_repo, upstream_repo)

        return not up_or_ahead

class NcsUpmerger(NcsWestCommand):
    def __init__(self):
        super().__init__(
            'ncs-upmerger',
            'tries to perform upmerge for a project',
            dedent('''
            Compares upstream and downstream project(s) with repo analyzer.
            If similar commits are found then tool will first revert these
            commits and after that performs merge of upstream.'''))

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description)
        add_projects_arg(parser)
        add_zephyr_rev_arg(parser)
        return parser

    def do_run(self, args: argparse.Namespace,
               unknown_args: list[str]) -> None:
        self.setup_upstream_downstream(args)
        for name, project in self.ncs_pmap.items():
            if name in self.z_pmap and name != 'manifest':
                z_project = self.z_pmap[name]
            elif name == 'zephyr':
                z_project = self.z_pmap['manifest']
            else:
                log.dbg(f'skipping downstream project {name}',
                        level=log.VERBOSE_VERY)
                continue
            self.upmerge(name, project, z_project)

    def upmerge(self, name: str, project: Project, z_project: Project) -> None:
        if name == 'zephyr':
            z_rev = self.zephyr_rev
        else:
            z_rev = z_project.revision
        n_sha = self.checked_sha(project, 'refs/heads/manifest-rev')
        z_sha = self.checked_sha(z_project, z_rev)

        log.inf(f'Starting upmerge for upstream sha: {z_sha} and downstream sha: {n_sha}')

        if not project.is_cloned() or n_sha is None or z_sha is None:
            if not project.is_cloned():
                log.wrn('project is not cloned; please run "west update"')
            elif n_sha is None:
                log.wrn(f"can't compare; please run \"west update {name}\" "
                        f'(need revision {project.revision})')
            elif z_sha is None:
                log.wrn(f"can't compare; please fetch upstream URL {z_project.url} "
                        f'(need revision {z_rev})')
            return

        try:
            analyzer = repo_analyzer(project, n_sha,
                                     z_project, z_sha)
        except nwh.InvalidRepositoryError as ire:
            log.die(f"{project.name_and_path}: {str(ire)}")

        for dc, ucs in reversed(analyzer.likely_merged.items()):
            if len(ucs) == 1:
                log.inf(f'- Reverting: {dc.oid} {commit_title(dc)}')
                log.inf(f'  Similar upstream title:\n'
                        f'  {ucs[0].oid} {commit_title(ucs[0])}')
            else:
                log.inf(f'- Reverting: {dc.oid} {commit_title(dc)}\n'
                        '  Similar upstream titles:')
                for i, uc in enumerate(ucs, start=1):
                    log.inf(f'    {i}. {uc.oid} {commit_title(uc)}')
            project.git('revert --signoff --no-edit ' + str(dc.oid))
        log.inf(f'Merging: {z_rev} to project: {project.name}')
        msg = f"[nrf mergeup] Merge upstream automatically up to commit {z_sha}\n\nThis auto-upmerge was performed with ncs-upmerger script."
        project.git('merge --no-edit --no-ff --signoff -m "' + msg + '" ' + str(self.zephyr_rev))

_UPSTREAM_ZEPHYR_URL = 'https://github.com/zephyrproject-rtos/zephyr'

# Set of project paths blocked from inclusion in the NCS.
_BLOCKED_PROJECTS: set[Path] = set(
    Path(p) for p in
    ['modules/audio/sof',
     'modules/debug/percepio',
     'modules/hal/altera',
     'modules/hal/ambiq',
     'modules/hal/atmel',
     'modules/hal/cypress',
     'modules/hal/espressif',
     'modules/hal/ethos_u',
     'modules/hal/gigadevice',
     'modules/hal/infineon',
     'modules/hal/intel',
     'modules/hal/microchip',
     'modules/hal/nuvoton',
     'modules/hal/nxp',
     'modules/hal/openisa',
     'modules/hal/qmsi',
     'modules/hal/quicklogic',
     'modules/hal/renesas',
     'modules/hal/rpi_pico',
     'modules/hal/silabs',
     'modules/hal/stm32',
     'modules/hal/telink',
     'modules/hal/ti',
     'modules/hal/xtensa',
     'modules/lib/acpica',
     'modules/crypto/mbedtls',
     'modules/lib/tflite-micro',
     'modules/lib/thrift',
     'modules/tee/tf-a/trusted-firmware-a',
     'modules/tee/tf-m/trusted-firmware-m',
     'tools/bsim',
     'tools/bsim/components',
     'tools/bsim/components/ext_2G4_libPhyComv1',
     'tools/bsim/components/ext_2G4_phy_v1',
     'tools/bsim/components/ext_2G4_channel_NtNcable',
     'tools/bsim/components/ext_2G4_channel_multiatt',
     'tools/bsim/components/ext_2G4_modem_magic',
     'tools/bsim/components/ext_2G4_modem_BLE_simple',
     'tools/bsim/components/ext_2G4_device_burst_interferer',
     'tools/bsim/components/ext_2G4_device_WLAN_actmod',
     'tools/bsim/components/ext_2G4_device_playback',
     'tools/bsim/components/ext_libCryptov1',
     ])
