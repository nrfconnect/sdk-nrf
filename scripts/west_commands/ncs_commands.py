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
from typing import Any, Optional

from west import log
from west.commands import WestCommand
from west.manifest import Manifest, MalformedManifest, ImportFlag, \
    MANIFEST_PROJECT_INDEX, Project
from west.version import __version__ as west_version
import yaml

try:
    from packaging import version
except ImportError:
    sys.exit("Can't import extra dependencies needed for NCS west extensions.\n"
             "Please install packages in nrf/scripts/requirements-extra.txt "
             "with pip3.")

import ncs_west_helpers as nwh
from pygit2_helpers import commit_affects_files, commit_title

WEST_V0_13_0_OR_LATER = version.parse(west_version) >= version.parse('0.13.0')

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

def likely_merged(np: Project, zp: Project, nsha: str, zsha: str) -> None:
    analyzer = repo_analyzer(np, nsha, zp, zsha)
    likely_merged = analyzer.likely_merged
    if likely_merged:
        # likely_merged is a map from downstream commits to
        # lists of upstream commits that look similar.
        log.msg('downstream patches which are likely merged upstream',
                '(revert these if appropriate):', color=log.WRN_COLOR)
        for dc, ucs in likely_merged.items():
            if len(ucs) == 1:
                log.inf(f'- {dc.oid} {commit_title(dc)}')
                log.inf(f'  Similar upstream title:\n'
                        f'  {ucs[0].oid} {commit_title(ucs[0])}')
            else:
                log.inf(f'- {dc.oid} {commit_title(dc)}\n'
                        '  Similar upstream titles:')
                for i, uc in enumerate(ucs, start=1):
                    log.inf(f'    {i}. {uc.oid} {commit_title(uc)}')
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
        if hasattr(args, 'projects'):
            try:
                projects = self.manifest.get_projects(
                    args.projects, only_cloned=True)
            except ValueError as ve:
                # West guarantees that get_projects()'s ValueError
                # has exactly two values in args.
                unknown, uncloned = ve.args
                if unknown:
                    log.die('unknown projects:', ', '.join(unknown))
                if uncloned:
                    log.die('uncloned downstream projects:',
                            ', '.join(u.name for u in uncloned) + '\n' +
                            'Run "west update", then retry.')
        else:
            projects = self.manifest.projects
        self.ncs_pmap = {p.name: p for p in projects}

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

        json_data: dict[str, Any] = {}
        for name, project in self.ncs_pmap.items():
            if name in self.z_pmap and name != 'manifest':
                z_project = self.z_pmap[name]
            elif name == 'zephyr':
                z_project = self.z_pmap['manifest']
            else:
                log.dbg(f'skipping downstream project {name}',
                        level=log.VERBOSE_VERY)
                continue
            self.print_loot(name, project, z_project, args, json_data)

        if args.json:
            with open(args.json, 'w') as f:
                json.dump(json_data, f)

    def print_loot(self, name: str, project: Project, z_project: Project,
                   args: argparse.Namespace, json_data: dict[str, Any]):
        # Print a list of out of tree outstanding patches in the given
        # project.
        #
        # name: project name
        # project: the west.manifest.Project instance in the NCS manifest
        # z_project: the Project instance in the upstream manifest
        # args: parsed arguments from argparse
        name_path = project.name_and_path

        # Get the upstream revision of the project. The zephyr project
        # has to be treated as a special case.
        if name == 'zephyr':
            z_rev = self.zephyr_rev
        else:
            z_rev = z_project.revision

        n_rev = 'refs/heads/manifest-rev'

        try:
            nsha = project.sha(n_rev)
            project.git('cat-file -e ' + nsha)
        except subprocess.CalledProcessError:
            log.wrn(f"{name_path}: can't get loot; please run "
                    f'"west update" (no "{n_rev}" ref)')
            return

        try:
            zsha = z_project.sha(z_rev)
            z_project.git('cat-file -e ' + zsha)
        except subprocess.CalledProcessError:
            log.wrn(f"{name_path}: can't get loot; please fetch upstream URL "
                    f'{z_project.url} (need revision {z_project.revision})')
            return

        try:
            analyzer = repo_analyzer(project, nsha,
                                     z_project, zsha)
        except nwh.InvalidRepositoryError as ire:
            log.die(f"{name_path}: {str(ire)}")

        try:
            loot = analyzer.downstream_outstanding
        except nwh.UnknownCommitsError as uce:
            log.die(f'{name_path}: unknown commits: {str(uce)}')

        if not loot and log.VERBOSE <= log.VERBOSE_NONE:
            # Don't print output if there's no loot unless verbose
            # mode is on.
            return

        log.banner(name_path)
        log.inf(f'     NCS commit: {nsha}\n'
                f'upstream commit: {zsha}')
        log.inf('OOT patches: ' +
                (f'{len(loot)} total' if loot else 'none') +
                (', output limited by --file' if args.files else ''))

        json_sha_list = []
        json_title_list = []
        for c in loot:
            if args.files and not commit_affects_files(c, args.files):
                log.dbg(f"skipping {c.oid}; it doesn't affect file filter",
                        level=log.VERBOSE_VERY)
                continue

            sha = str(c.oid)
            title = commit_title(c)
            if args.sha_only:
                log.inf(sha)
            else:
                log.inf(f'- {sha} {title}')

            if args.json:
                json_sha_list.append(sha)
                json_title_list.append(title)

        if args.json:
            json_data[name] = {
                'path': project.path,
                'ncs-commit': nsha,
                'upstream-commit': zsha,
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
        for zp in self.z_pmap.values():
            nn = to_ncs_name(zp)
            present = nn in self.ncs_pmap
            blocked = Path(zp.path) in _BLOCKED_PROJECTS
            if present:
                if blocked:
                    present_blocked.append(zp)
                else:
                    present_allowed.append(zp)
            else:
                if blocked:
                    missing_blocked.append(zp)
                else:
                    missing_allowed.append(zp)

        def print_lst(projects):
            for p in projects:
                log.inf(f'{p.name_and_path}')

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
            for p in missing_allowed:
                log.small_banner(f'{p.name_and_path}:')
                log.inf(f'upstream revision: {p.revision}')
                log.inf(f'upstream URL: {p.url}')
        else:
            log.inf('none (OK)')

        if present_allowed:
            log.banner('projects in both zephyr and NCS:')
            for zp in present_allowed:
                # Do some extra checking on unmerged commits.
                self.allowed_project(zp)

        if log.VERBOSE <= log.VERBOSE_NONE:
            log.inf('\nNote: verbose output was omitted,',
                    'use "west -v ncs-compare" for more details.')

    def allowed_project(self, zp: Project) -> None:
        nn = to_ncs_name(zp)
        np = self.ncs_pmap[nn]
        # is_imported is true if we imported this project from the
        # zephyr manifest rather than defining it directly ourselves
        # in nrf/west.yml.
        is_imported = nn in self.imported_pmap
        imported = ', imported from zephyr' if is_imported else ''
        banner = f'{nn} ({zp.path}){imported}:'

        nrev = 'refs/heads/manifest-rev'
        if np.name == 'zephyr':
            zrev = self.zephyr_sha
        else:
            zrev = zp.revision

        nsha = self.checked_sha(np, nrev)
        zsha = self.checked_sha(zp, zrev)

        if not np.is_cloned() or nsha is None or zsha is None:
            log.small_banner(banner)
            if not np.is_cloned():
                log.wrn('project is not cloned; please run "west update"')
            elif nsha is None:
                log.wrn(f"can't compare; please run \"west update {nn}\" "
                        f'(need revision {np.revision})')
            elif zsha is None:
                log.wrn(f"can't compare; please fetch upstream URL {zp.url} "
                        f'(need revision {zp.revision})')
            return

        cp = np.git(f'rev-list --left-right --count {zsha}...{nsha}',
                    capture_stdout=True)
        behind, ahead = [int(c) for c in cp.stdout.split()]

        if zsha == nsha:
            status = 'up to date'
        elif ahead and not behind:
            status = f'ahead by {ahead} commit' + ("s" if ahead > 1 else "")
        elif np.is_ancestor_of(nsha, zsha):
            status = f'behind by {behind} commit' + ("s" if behind > 1 else "")
        else:
            status = f'diverged: {ahead} ahead, {behind} behind'

        commits = (f'     NCS commit: {nsha}\n'
                   f'upstream commit: {zsha}')
        if 'up to date' in status or 'ahead by' in status:
            if log.VERBOSE > log.VERBOSE_NONE:
                # Up to date or ahead: only print in verbose mode.
                log.small_banner(banner)
                log.inf(commits)
                log.inf(status)
                likely_merged(np, zp, nsha, zsha)
        else:
            # Behind or diverged: always print.
            if is_imported and 'behind by' in status:
                status += ' and imported: update by doing zephyr mergeup'

            log.small_banner(banner)
            log.inf(commits)
            log.msg(status, color=log.WRN_COLOR)
            likely_merged(np, zp, nsha, zsha)

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
            project.git('revert --no-edit ' + str(dc.oid))
        log.inf(f'Merging: {z_rev} to project: {project.name}')
        msg = f"[nrf mergeup] Merge upstream automatically up to commit {z_sha}\n\nThis auto-upmerge was performed with ncs-upmerger script."
        project.git('merge --no-edit --no-ff --signoff -m "' + msg + '" ' + str(self.zephyr_rev))

_UPSTREAM_ZEPHYR_URL = 'https://github.com/zephyrproject-rtos/zephyr'

# Set of project paths blocked from inclusion in the NCS.
_BLOCKED_PROJECTS: set[Path] = set(
    Path(p) for p in
    ['modules/audio/sof',
     'modules/hal/altera',
     'modules/hal/atmel',
     'modules/hal/cypress',
     'modules/hal/espressif',
     'modules/hal/ethos_u',
     'modules/hal/gigadevice',
     'modules/hal/infineon',
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
     'modules/lib/tflite-micro',
     'modules/tee/tf-a/trusted-firmware-a',
     ])
