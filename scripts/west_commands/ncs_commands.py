# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''The "ncs-xyz" extension commands.'''

from pathlib import PurePath
import subprocess
from textwrap import dedent

import packaging.version
from west import log
from west.commands import WestCommand
from west.manifest import Manifest, MalformedManifest
from west.version import __version__ as west_ver
import yaml

import ncs_west_helpers as nwh

def add_zephyr_rev_arg(parser):
    parser.add_argument('-z', '--zephyr-rev',
                        help='''upstream git ref to use for zephyr project;
                        default is upstream/master''')

def add_projects_arg(parser):
    parser.add_argument('projects', metavar='PROJECT', nargs='*',
                        help='''projects (by name or path) to operate on;
                        defaults to all projects in the manifest which
                        are shared with the upstream manifest''')

def check_west_version():
    min_ver = '0.6.99'
    if (packaging.version.parse(west_ver) <
            packaging.version.parse(min_ver)):
        log.die('this command currently requires a pre-release west '
                f'(your west version is {west_ver}, '
                f'but must be at least {min_ver}')

def likely_merged(np, zp, nsha, zsha):
    analyzer = nwh.RepoAnalyzer(np, zp, nsha, zsha)
    likely_merged = analyzer.likely_merged
    if likely_merged:
        # likely_merged is a map from downstream commits to
        # lists of upstream commits that look similar.
        log.msg('downstream patches which are likely merged upstream',
                '(revert these if appropriate):', color=log.WRN_COLOR)
        for dc, ucs in likely_merged.items():
            log.inf(f'- {dc.oid} ({nwh.commit_shortlog(dc)})\n'
                    '  Similar upstream commits:')
            for uc in ucs:
                log.inf(f'  {uc.oid} ({nwh.commit_shortlog(uc)})')
    else:
        log.inf('no downstream patches seem to have been merged upstream')

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
        except subprocess.CalledProcessError:
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
            # The topdir kwarg was added in a pre-release west, which
            # is required to use this file. The latest stable (0.6.3)
            # doesn't have it, so pylint is failing with a false error
            # report here. Turn it off for now; this can be removed
            # when west 0.7 is out.
            #
            # pylint: disable=unexpected-keyword-arg
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
        parser = parser_adder.add_parser(self.name, help=self.help,
                                         description=self.description)
        add_zephyr_rev_arg(parser)
        parser.add_argument('-s', '--sha', dest='sha_only',
                            action='store_true',
                            help='only print SHAs of OOT commits')
        parser.add_argument('-f', '--file', dest='files', action='append',
                            help='''restrict output to patches touching
                            this file; may be given multiple times. If used,
                            a single project must be given''')
        add_projects_arg(parser)
        return parser

    def do_run(self, args, unknown_args):
        check_west_version()
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
        log.banner(f'{_name_and_path(project)}')

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
            log.wrn(f"can't get loot; please run "
                    f'"west update {project.name}" '
                    f'(need revision {project.revision})')
            return
        try:
            zsha = z_project.sha(z_rev)
            z_project.git('cat-file -e ' + zsha)
        except subprocess.CalledProcessError:
            log.wrn("can't get loot; please fetch upstream URL "
                    f'{z_project.url} '
                    f'(need revision {z_project.revision})')
            return

        log.inf(f'NCS commit: {nsha}\nupstream commit: {zsha}')
        try:
            analyzer = nwh.RepoAnalyzer(project, z_project,
                                        project.revision, z_rev)
        except nwh.InvalidRepositoryError as ire:
            log.die(str(ire))
        try:
            loot = analyzer.downstream_outstanding
            log.inf('OOT patches: ' +
                    (f'{len(loot)} total ' if loot else 'none'))
            for c in loot:
                if args.files and not nwh.commit_affects_files(c, args.files):
                    log.dbg(f"skipping {c.oid}; it doesn't affect file filter",
                            level=log.VERBOSE_VERY)
                    continue
                if args.sha_only:
                    log.inf(str(c.oid))
                else:
                    log.inf(f'- {c.oid} {nwh.commit_shortlog(c)}')
        except nwh.UnknownCommitsError as uce:
            log.die('unknown commits:', str(uce))

class NcsCompare(NcsWestCommand):
    def __init__(self):
        super().__init__(
            'ncs-compare',
            'compare upstream manifest with NCS',
            dedent('''
            Compares project status in upstream and NCS manifests.

            Run this after doing a zephyr mergeup to double-check the
            results. This command works using the current contents of
            zephyr/west.yml, so the mergeup must be complete for output
            to be valid.

            Use --verbose to include information on blacklisted
            upstream projects.'''))

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(self.name, help=self.help,
                                         description=self.description)
        add_zephyr_rev_arg(parser)
        return parser

    def do_run(self, args, unknown_args):
        check_west_version()
        self.setup_upstream_downstream(args)

        log.inf('Comparing nrf/west.yml with zephyr/west.yml at revision '
                f'{self.zephyr_rev}' +
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
                log.small_banner(f'{_name_and_path(p)}')

        if missing_blacklisted and log.VERBOSE >= log.VERBOSE_NORMAL:
            log.banner('blacklisted zephyr projects',
                       'not in nrf (these are all OK):')
            print_lst(missing_blacklisted)

        log.banner('blacklisted zephyr projects in nrf:')
        if present_blacklisted:
            log.wrn('these should all be removed from nrf')
            print_lst(present_blacklisted)
        else:
            log.inf('none (OK)')

        log.banner('non-blacklisted zephyr projects',
                   'missing from nrf:')
        if missing_allowed:
            log.wrn('these should be blacklisted or added to nrf')
            for p in missing_allowed:
                log.small_banner(f'{_name_and_path(p)}:')
                log.inf(f'upstream revision: {p.revision}')
                log.inf(f'upstream URL: {p.url}')
        else:
            log.inf('none (OK)')

        if present_allowed:
            log.banner('projects in both zephyr and nrf:')
            for zp in present_allowed:
                # Do some extra checking on unmerged commits.
                self.allowed_project(zp)

        if log.VERBOSE <= log.VERBOSE_NONE:
            log.inf('\nNote: verbose output was omitted,',
                    'use "west -v ncs-compare" for more details.')

    def allowed_project(self, zp):
        nn = to_ncs_name(zp)
        np = self.ncs_pmap[nn]
        banner = f'{nn} ({zp.path}):'

        if np.name == 'zephyr':
            nrev = self.manifest.get_projects(['zephyr'])[0].revision
            zrev = self.zephyr_sha
        else:
            nrev = np.revision
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
            status = f'ahead by {ahead} commits'
        elif np.is_ancestor_of(nsha, zsha):
            status = (f'behind by {behind} commits, '
                      f'can be fast-forwarded to {zsha}')
        else:
            status = f'diverged from upstream: {ahead} ahead, {behind} behind'

        upstream_rev = f'upstream revision: {zrev}'
        if zrev != zsha:
            upstream_rev += f' ({zsha})'

        downstream_rev = 'NCS revision: ' + nrev
        if nrev != nsha:
            downstream_rev += f' ({nsha})'

        if 'up to date' in status or 'ahead by' in status:
            if log.VERBOSE > log.VERBOSE_NONE:
                # Up to date or ahead: only print in verbose mode.
                log.small_banner(banner)
                status += ', ' + downstream_rev
                if 'ahead by' in status:
                    status += ', ' + upstream_rev
                log.inf(status)
                likely_merged(np, zp, nsha, zsha)
        else:
            # Behind or diverged: always print.
            log.small_banner(banner)
            log.msg(status, color=log.WRN_COLOR)
            log.inf(upstream_rev)
            log.inf(downstream_rev)
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
