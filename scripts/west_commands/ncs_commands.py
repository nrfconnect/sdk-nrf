# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''The "ncs-xyz" extension commands.'''

import argparse
from pathlib import PurePath
import subprocess
from textwrap import dedent

from west import log
from west.commands import WestCommand
from west.manifest import Manifest, MalformedManifest, ImportFlag, \
    MANIFEST_PROJECT_INDEX
import yaml

import ncs_west_helpers as nwh

def add_zephyr_rev_arg(parser):
    parser.add_argument('-z', '--zephyr-rev', metavar='REF',
                        help='''zephyr git ref (commit, branch, etc.);
                        default: upstream/master''')

def add_projects_arg(parser):
    parser.add_argument('projects', metavar='PROJECT', nargs='*',
                        help='projects (by name or path) to operate on')

def likely_merged(np, zp, nsha, zsha):
    analyzer = nwh.RepoAnalyzer(np, zp, nsha, zsha)
    likely_merged = analyzer.likely_merged
    if likely_merged:
        # likely_merged is a map from downstream commits to
        # lists of upstream commits that look similar.
        log.msg('downstream patches which are likely merged upstream',
                '(revert these if appropriate):', color=log.WRN_COLOR)
        for dc, ucs in likely_merged.items():
            if len(ucs) == 1:
                log.inf(f'- {dc.oid} {nwh.commit_shortlog(dc)}')
                log.inf(f'  Similar upstream shortlog:\n'
                        f'  {ucs[0].oid} {nwh.commit_shortlog(ucs[0])}')
            else:
                log.inf(f'- {dc.oid} {nwh.commit_shortlog(dc)}\n'
                        '  Similar upstream shortlogs:')
                for i, uc in enumerate(ucs, start=1):
                    log.inf(f'    {i}. {uc.oid} {nwh.commit_shortlog(uc)}')
    else:
        log.dbg('no downstream patches seem to have been merged upstream')

def to_ncs_name(zp):
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
    def checked_sha(project, revision):
        # get revision's SHA and check that it exists in the project.
        # returns the SHA on success, or None if we can't unwrap revision
        # to a SHA which is present in the project.

        try:
            sha = project.sha(revision)
            project.git('cat-file -e ' + sha)
            return sha
        except (subprocess.CalledProcessError, FileNotFoundError):
            return None

    def setup_upstream_downstream(self, args):
        # set some instance state that will be useful for
        # comparing upstream and downstream.
        #
        # below, "pmap" means "project map": a dict mapping project
        # names to west.manifest.Project instances.

        # z_manifest: upstream zephyr west.manifest.Manifest instance
        #
        # zephyr_rev: validated --zephyr-rev argument.
        # (quits and print a message about what to do if things go wrong).
        if hasattr(args, 'zephyr_rev'):
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
                #
                # pylint: disable=unbalanced-tuple-unpacking
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

    def zephyr_manifest(self):
        # load the upstream manifest. the west.manifest APIs guarantee
        # in this case that its project hierarchy is rooted in the NCS
        # directory.

        z_project = self.manifest.get_projects(['zephyr'],
                                               allow_paths=False,
                                               only_cloned=True)[0]
        cp = z_project.git(f'show {self.zephyr_rev}:west.yml',
                           capture_stdout=True, check=True)
        z_west_yml = cp.stdout.decode('utf-8')
        try:
            return Manifest.from_data(source_data=yaml.safe_load(z_west_yml),
                                      topdir=self.topdir)
        except MalformedManifest:
            log.die(f"can't load zephyr manifest; file {z_west_yml} "
                    "is malformed")

    def validate_zephyr_rev(self, args):
        if args.zephyr_rev:
            zephyr_rev = args.zephyr_rev
        else:
            zephyr_rev = 'upstream/master'
        zephyr_project = self.manifest.get_projects(['zephyr'])[0]
        try:
            self.zephyr_sha = zephyr_project.sha(zephyr_rev)
        except subprocess.CalledProcessError:
            msg = f'unknown ZEPHYR_REV: {zephyr_rev}'
            if not args.zephyr_rev:
                msg += ('\nPlease run this in the zephyr repository and retry '
                        '(or use --zephyr-rev):' +
                        '\n  git remote add -f upstream ' + _UPSTREAM)
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
        parser.add_argument('-f', '--file', dest='files', metavar='FILE',
                            action='append',
                            help='''list patches changing this file;
                            may be given multiple times''')
        add_projects_arg(parser)
        return parser

    def do_run(self, args, unknown_args):
        if args.files and len(args.projects) != 1:
            self.parser.error('--file used, so expected 1 project argument, '
                              f'but got: {len(args.projects)}')
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
            self.print_loot(name, project, z_project, args)

    def print_loot(self, name, project, z_project, args):
        # Print a list of out of tree outstanding patches in the given
        # project.
        #
        # name: project name
        # project: the west.manifest.Project instance in the NCS manifest
        # z_project: the Project instance in the upstream manifest
        name_path = _name_and_path(project)

        # Get the upstream revision of the project. The zephyr project
        # has to be treated as a special case.
        if name == 'zephyr':
            z_rev = self.zephyr_rev
        else:
            z_rev = z_project.revision

        try:
            nsha = project.sha(project.revision)
            project.git('cat-file -e ' + nsha)
        except subprocess.CalledProcessError:
            log.wrn(f"{name_path}: can't get loot; please run "
                    f'"west update" (need revision {project.revision})')
            return
        try:
            zsha = z_project.sha(z_rev)
            z_project.git('cat-file -e ' + zsha)
        except subprocess.CalledProcessError:
            log.wrn(f"{name_path}: can't get loot; please fetch upstream URL "
                    f'{z_project.url} (need revision {z_project.revision})')
            return

        try:
            analyzer = nwh.RepoAnalyzer(project, z_project,
                                        project.revision, z_rev)
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
        log.inf(f'NCS commit: {nsha}, upstream commit: {zsha}')
        log.inf('OOT patches: ' +
                (f'{len(loot)} total' if loot else 'none') +
                (', output limited by --file' if args.files else ''))
        for c in loot:
            if args.files and not nwh.commit_affects_files(c, args.files):
                log.dbg(f"skipping {c.oid}; it doesn't affect file filter",
                        level=log.VERBOSE_VERY)
                continue
            if args.sha_only:
                log.inf(str(c.oid))
            else:
                log.inf(f'- {c.oid} {nwh.commit_shortlog(c)}')

class NcsCompare(NcsWestCommand):
    def __init__(self):
        super().__init__(
            'ncs-compare',
            'compare upstream manifest with NCS',
            dedent('''
            Compares project status between zephyr/west.yml and your
            current NCS west manifest-rev branches (i.e. the results of your
            most recent 'west update').

            Use --verbose to include information on blacklisted
            upstream projects.'''))

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description)
        add_zephyr_rev_arg(parser)
        return parser

    def do_run(self, args, unknown_args):
        self.setup_upstream_downstream(args)

        # Get a dict containing projects that are in the NCS which are
        # *not* imported from Zephyr in nrf/west.yml. We will treat
        # these specially to make the output easier to understand.
        ignored_imports = Manifest.from_file(
            import_flags=ImportFlag.IGNORE_PROJECTS)
        in_nrf = set(p.name for p in
                     ignored_imports.projects[MANIFEST_PROJECT_INDEX + 1:])
        # This is a dict mapping names of projects which *are* imported
        # from zephyr to the Project instances.
        self.imported_pmap = {name: project for name, project in
                              self.ncs_pmap.items() if name not in in_nrf}

        log.inf('Comparing your manifest-rev branches with zephyr/west.yml '
                f'at {self.zephyr_rev}' +
                (', sha: ' + self.zephyr_sha
                 if self.zephyr_rev != self.zephyr_sha else ''))
        log.inf()

        present_blacklisted = []
        present_allowed = []
        missing_blacklisted = []
        missing_allowed = []
        for zp in self.z_pmap.values():
            nn = to_ncs_name(zp)
            present = nn in self.ncs_pmap
            blacklisted = PurePath(zp.path) in _PROJECT_BLACKLIST
            if present:
                if blacklisted:
                    present_blacklisted.append(zp)
                else:
                    present_allowed.append(zp)
            else:
                if blacklisted:
                    missing_blacklisted.append(zp)
                else:
                    missing_allowed.append(zp)

        def print_lst(projects):
            for p in projects:
                log.inf(f'{_name_and_path(p)}')

        if missing_blacklisted and log.VERBOSE >= log.VERBOSE_NORMAL:
            log.banner('blacklisted zephyr projects',
                       'not in nrf (these are all OK):')
            print_lst(missing_blacklisted)

        log.banner('blacklisted zephyr projects in NCS:')
        if present_blacklisted:
            log.wrn(f'these should all be removed from {self.manifest.path}!')
            print_lst(present_blacklisted)
        else:
            log.inf('none (OK)')

        log.banner('non-blacklisted zephyr projects missing from NCS:')
        if missing_allowed:
            west_yml = self.manifest.path
            log.wrn(
                f'missing projects should be added to NCS or blacklisted\n'
                f"  To add to NCS:\n"
                f"    1. do the zephyr mergeup\n"
                f"    2. update zephyr revision in {west_yml}\n"
                f"    3. add projects to zephyr's name_whitelist in "
                f"{west_yml}\n"
                f"    4. run west {self.name} again to check your work\n"
                f"  To blacklist: edit _PROJECT_BLACKLIST in {__file__}")
            for p in missing_allowed:
                log.small_banner(f'{_name_and_path(p)}:')
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

    def allowed_project(self, zp):
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

        commits = f'NCS commit: {nsha}, upstream commit: {zsha}'
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

def _name_and_path(project):
    # This is just a compatibility shim to keep things going until we
    # can rely on project.name_and_path's availability in west 0.7.

    return f'{project.name} ({project.path})'

_UPSTREAM = 'https://github.com/zephyrproject-rtos/zephyr'

# Set of project paths blacklisted from inclusion in the NCS.
_PROJECT_BLACKLIST = set(
    PurePath(p) for p in
    ['modules/hal/atmel',
     'modules/hal/esp-idf',
     'modules/hal/qmsi',
     'modules/hal/cypress',
     'modules/hal/openisa',
     'modules/hal/microchip',
     'modules/hal/silabs',
     'modules/hal/stm32',
     'modules/hal/ti',
     'modules/hal/nxp',
     'modules/hal/xtensa'])
