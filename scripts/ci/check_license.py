#!/usr/bin/env python3
#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Check for allowed licenses in a "sdk-nrf" pull request.

For the "sdk-nrf" repository it checks the files changed by the pull request. In addition, when the
pull request modifies the west manifest (west.yml) it also checks the new files brought in by the
modules:
  1. Parses the manifest at the base and at the PR head.
  2. Classifies which manifest projects were updated or added considering only projects in the
     "nrfconnect" org (classify_project_changes).
  3. Collects the files to check:
       - updated project: only files added between the old and the new revision
       - added project: all files at the new revision
  4. Detects the licenses of all collected files with "west ncs-sbom" and checks them against the
     allow list.
'''

import argparse
import contextlib
import json
import re
import shlex
import subprocess
import sys
import tempfile
import textwrap
from pathlib import Path

import junit_xml
import yaml
from west.manifest import ImportFlag, Manifest

NRFCONNECT_URL_PREFIX = 'https://github.com/nrfconnect/'

# Messages shown in the output
LICENSE_ALLOWED = '"*" license is allowed for this file.'
NONE_LICENSE_ALLOWED = 'Missing license information is allowed for this file.'
ANY_LICENSE_ALLOWED = 'Any license is allowed for this file.'
LICENSE_WARNING = '"*" license is allowed for this file, but it is not recommended.'
NONE_LICENSE_WARNING = ('Missing license information is allowed for this file, but it is '
                        'recommended to add one.')
ANY_LICENSE_WARNING = ('Any license is allowed for this file, but it is recommended to use a more '
                       'suitable one.')
LICENSE_ERROR = '"*" license is not allowed for this file.'
NONE_LICENSE_ERROR = 'Missing license information is not allowed for this file.'
SKIP_MISSING_FILE_TEXT = 'The file does not exist anymore.'
SKIP_DIRECTORY_TEXT = 'This is a directory.'
SKIP_EXTERNAL_LICENSE_TEXT = 'External license metadata is not checked.'
EXTERNAL_LICENSE_DIR = Path('nrf/scripts/west_commands/sbom/data/external-licenses')
RECOMMENDATIONS_TEXT = textwrap.dedent('''\
    ===============================================================================
    You have some license problems. Check the following:
    * The files in the commits are covered by a license allowed in the nRF Connect
      SDK.
    * The source files have a correct "SPDX-License-Identifier" tag.
    * The libraries have an associated external license file and the tags contained
      in it are correct. For details, see documentation for the Software Bill of
      Materials script.
    ===============================================================================
''')


def parse_args():
    '''Parse command line arguments.'''
    default_allow_list = Path(__file__).parent / 'license_allow_list.yaml'
    parser = argparse.ArgumentParser(
        description='Check for allowed licenses.',
        allow_abbrev=False)
    parser.add_argument('-c', '--commits', default='HEAD~1..',
                        help='Commit range in the form: a..[b], default is HEAD~1..HEAD')
    parser.add_argument('-o', '--output', type=Path, default='licenses.xml',
                        help='''Name of outfile in JUnit format, default is ./licenses.xml''')
    parser.add_argument('-l', '--allow-list', type=Path, default=default_allow_list,
                        help=f'Allow list file, default is {default_allow_list}')
    parser.add_argument('--github', action='store_true',
                        help='Add GitHub Actions Workflow commands to the stdout.')
    parser.add_argument('-m', '--manifest', default='west.yml',
                        help='Manifest file relative to the repository, default is west.yml',)
    return parser.parse_args()


def unlink_quietly(path: Path) -> None:
    '''Delete a file if it exists.'''
    with contextlib.suppress(FileNotFoundError):
        path.unlink()


def is_external_license_file(file_path: Path) -> bool:
    '''Return True for external license metadata files that are not project files.'''
    file_str = Path(file_path).as_posix()
    prefix = EXTERNAL_LICENSE_DIR.as_posix().rstrip('/') + '/'
    return file_str.startswith(prefix)


def classify_project_changes(old_items: set, new_items: set) -> 'tuple[set, set, set]':
    '''
    Classify the difference between two sets of (name, revision) tuples.

    Returns (removed, updated, added):
      * removed: items whose name is no longer present,
      * updated: items whose name is present in both but with a different revision,
      * added:   items whose name is new.

    This mirrors the logic of zephyrproject-rtos/action-manifest "_get_sets".
    '''
    # Removed items.
    ritems = set(filter(lambda p: p[0] not in list(q[0] for q in new_items), old_items - new_items))
    # Updated items.
    uitems = set(filter(lambda p: p[0] in list(q[0] for q in old_items), new_items - old_items))
    # Added items.
    aitems = new_items - old_items - uitems
    return (ritems, uitems, aitems)


class FileLicenseChecker:
    '''Class that checks if a license is allowed for a file.'''

    allow_list: 'dict[str, list[tuple[re.Pattern, bool]]]'

    def __init__(self, allow_list_file: Path):
        with open(allow_list_file) as fd:
            data = yaml.safe_load(fd)
        self.allow_list = {}
        for key in data:
            self.allow_list[key.upper()] = self.parse_re(data[key])

    @staticmethod
    def parse_re(re_str: str) -> 'list[tuple[re.Pattern, bool]]':
        '''
        Convert a multiline string to a list of regular expressions and boolean indicating if it is
        negative match.
        '''
        result = []
        lines = re_str.strip().splitlines()
        for line in lines:
            line = line.strip()
            if not line:
                continue
            if line[0] == '!':
                value = False
                line = line[1:].strip()
            else:
                value = True
            result.append((re.compile(line), value))
        return result

    def check(self, license_identifier: str, file_path: 'str|Path') -> bool:
        '''
        Check if a license is allowed for a file. The file is relative to the west workspace.
        The license identifier can be prefixed with a '-' to check if the license is allowed,
        but with a warning.
        '''
        file_path = str(file_path).replace('\\', '/')
        license_identifier = license_identifier.upper()
        if license_identifier not in self.allow_list:
            return False
        allow = False
        for pattern, value in self.allow_list[license_identifier]:
            if pattern.search(file_path):
                allow = value
        return allow


class PatchLicenseChecker:
    '''Check licenses for a git patch and for the modules updated by a west.yml change.'''

    args: dict
    license_checker: FileLicenseChecker
    git_top: Path
    west_workspace: Path
    junit_test_cases: str
    total_tests: int
    total_skipped: int
    total_errors: int
    total_warnings: int

    def __init__(self, args: dict):
        self.args = args
        self.license_checker = FileLicenseChecker(args.allow_list)

    def run(self, program: str, *args: 'list[str|Path]', cwd=None) -> str:
        '''A helper function to run an external program.'''
        run_cmd = (program,) + tuple(str(a) for a in args)
        run_str = ' '.join(shlex.quote(arg) for arg in run_cmd)
        try:
            process = subprocess.Popen(run_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                       cwd=cwd)
        except OSError as e:
            self.report('error', f'Failed to run "{run_str}": {e}')
            self.write_junit()
            sys.exit(2)
        stdout, stderr = process.communicate()
        stdout = stdout.decode('utf-8')
        stderr = stderr.decode('utf-8')
        if process.returncode or stderr:
            self.report('error', f'Command "{run_str}" exited with {process.returncode}\n'
                                 f'==stdout==\n{stdout}\n==stderr==\n{stderr}')
            self.write_junit()
            sys.exit(2)
        return stdout.rstrip()

    def try_run(self, program: str, *args: 'list[str|Path]', cwd=None):
        '''Run an external program without failing the whole check on a non-zero exit.'''
        run_cmd = (program,) + tuple(str(a) for a in args)
        return subprocess.run(run_cmd, capture_output=True, text=True, cwd=cwd)

    def report(self, label: str, message: str, file_name: 'str|Path' = '<none>', license: str = ''):
        '''Report results to the user.'''
        file_name = str(file_name)
        # Print to stdout/stderr with optional GitHub Actions Workflow commands.
        if not self.args.github:
            if label in ('error', 'warning'):
                print(f'{label.upper()}: {file_name}: {message}', file=sys.stderr)
            else:
                print(f'{label.upper()}: {file_name}: {message}')
        else:
            print(f'{file_name}: ')
            if label in ('error', 'warning'):
                if file_name != '<none>':
                    try:
                        file_path = (self.west_workspace / file_name).relative_to(self.git_top)
                    except ValueError:
                        # A module file (outside the "sdk-nrf" repository) is already relative to
                        # the west workspace; keep it as is.
                        file_path = file_name
                else:
                    file_path = file_name
                print(f'::{label} file={file_path},title=License Problem::' +
                      message.replace('%', '%25').replace('\r', '%0D').replace('\n', '%0A'))
            else:
                print(f'{label.upper()}: {message}')
        # Put result in JUnit file.
        test_case = junit_xml.TestCase(file_name + (f':{license}' if license else ''),
                                       'LicenseCheck')
        if label == 'error':
            test_case.add_failure_info(message)
        elif label == 'warning':
            test_case.add_skipped_info('WARNING: ' + message)
        elif label == 'skip':
            test_case.add_skipped_info(message)
        self.junit_test_cases.append(test_case)
        # Increment counters.
        self.total_tests += 1
        if label == 'error':
            self.total_errors += 1
        elif label == 'warning':
            self.total_warnings += 1
        elif label == 'skip':
            self.total_skipped += 1

    def generate_list_of_files(self) -> 'list[Path]':
        '''
        Generate a list of files changed in the git patch. The returned path is relative to the
        west workspace.
        '''
        files = self.run('git', 'diff', '--name-only', '--diff-filter=d', self.args.commits)
        files = files.splitlines()
        files = [(self.git_top / f.strip()).relative_to(self.west_workspace)
                 for f in files if f.strip()]
        return files

    def commit_range(self) -> 'tuple[str, str]':
        '''Split the "--commits" into the base and head revisions.'''
        parts = self.args.commits.split('..')
        base = parts[0]
        head = parts[1] if len(parts) > 1 and parts[1] else 'HEAD'
        return base, head

    def load_projects(self, manifest_data: str) -> 'dict':
        '''
        parses one west.yml manifest and returns only the relevant projects keeping only
        active projects hosted under the NRFCONNECT_URL_PREFIX org.
        '''
        manifest = Manifest.from_data(manifest_data, import_flags=ImportFlag.IGNORE)
        projects = {}
        for project in manifest.projects:
            url = (project.url or '').lower().removesuffix('.git')
            if not url.startswith(NRFCONNECT_URL_PREFIX):
                continue
            if not manifest.is_active(project):
                continue
            projects[project.name] = project
        return projects

    def ensure_revision(self, project_dir: Path, revision: str) -> bool:
        '''Make sure a revision is available locally, fetching it if necessary.'''
        if (
            self.try_run(
                'git', 'cat-file', '-e', f'{revision}^{{commit}}', cwd=project_dir
            ).returncode
            == 0
        ):
            return True
        self.try_run('git', 'fetch', '--no-tags', 'origin', revision, cwd=project_dir)
        return (
            self.try_run(
                'git', 'cat-file', '-e', f'{revision}^{{commit}}', cwd=project_dir
            ).returncode
            == 0
        )

    def prefix(self, project_path: str, git_output: str) -> 'list[str]':
        '''Prefix each git output line with the project path to make it workspace relative.'''
        return [
            f'{project_path}/{line.strip()}' for line in git_output.splitlines() if line.strip()
        ]

    def added_files(self, project, old_rev: str, new_rev: str) -> 'list[str]':
        '''Return the workspace relative paths of files added between old_rev and new_rev.'''
        project_dir = self.west_workspace / project.path
        if not (
            self.ensure_revision(project_dir, old_rev)
            and self.ensure_revision(project_dir, new_rev)
        ):
            return []
        out = self.run(
            'git',
            'diff',
            '--name-only',
            '--diff-filter=A',
            f'{old_rev}..{new_rev}',
            cwd=project_dir,
        )
        return self.prefix(project.path, out)

    def all_files(self, project, new_rev: str) -> 'list[str]':
        '''Return the workspace relative paths of all files at new_rev (newly added project).'''
        project_dir = self.west_workspace / project.path
        if not self.ensure_revision(project_dir, new_rev):
            return []
        out = self.run('git', 'ls-tree', '-r', '--name-only', new_rev, cwd=project_dir)
        return self.prefix(project.path, out)

    def collect_manifest_files(self) -> 'list[Path]':
        '''
        If the manifest changed collect the new files of the updated and added
        NRFCONNECT_URL_PREFIX projects.
        '''
        manifest_path = self.args.manifest
        changed = self.run('git', 'diff', '--name-only', self.args.commits, '--', manifest_path)
        if not changed.strip():
            print(f'"{manifest_path}" was not modified; skipping module license checks.')
            return []

        base, head = self.commit_range()
        old_projects = self.load_projects(self.run('git', 'show', f'{base}:{manifest_path}'))
        new_projects = self.load_projects(self.run('git', 'show', f'{head}:{manifest_path}'))

        old_set = {(p.name, p.revision) for p in old_projects.values()}
        new_set = {(p.name, p.revision) for p in new_projects.values()}
        (removed, updated, added) = classify_project_changes(old_set, new_set)
        print(f'Removed projects: {sorted(n for n, _ in removed)}')
        print(f'Updated projects: {sorted(n for n, _ in updated)}')
        print(f'Added projects:   {sorted(n for n, _ in added)}')

        if not updated and not added:
            return []

        files = []
        for name, _ in sorted(updated):
            project = new_projects[name]
            old_rev = old_projects[name].revision
            new_rev = project.revision
            print(
                f'Updated project "{name}": collecting files added in {old_rev[:12]}..'
                f'{new_rev[:12]}'
            )
            files += self.added_files(project, old_rev, new_rev)
        for name, _ in sorted(added):
            project = new_projects[name]
            print(f'Added project "{name}": collecting the entire tree at {project.revision[:12]}')
            files += self.all_files(project, project.revision)
        return [Path(f) for f in files]

    def skip_files(self, files: 'list[Path]') -> 'list[Path]':
        '''
        Remove files from the list, because they do not exist, or they can have any license.
        A new list is returned.
        '''
        new_list = []
        for file_name in files:
            if not (self.west_workspace / file_name).exists():
                self.report('skip', SKIP_MISSING_FILE_TEXT, file_name)
            if not (self.west_workspace / file_name).is_file():
                self.report('skip', SKIP_DIRECTORY_TEXT, file_name)
            elif is_external_license_file(file_name):
                self.report('skip', SKIP_EXTERNAL_LICENSE_TEXT, file_name)
            elif self.license_checker.check('ANY', file_name):
                self.report('skip', ANY_LICENSE_ALLOWED, file_name)
            else:
                new_list.append(file_name)
        return new_list

    def detect_licenses(self, files: 'list[Path]') -> 'list[dict]':
        '''Use the "west ncs-sbom" command to detect the licenses of the files.'''
        with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8', suffix='.txt', prefix='_tmp',
                                         dir=self.west_workspace, delete=False) as tmp:
            for file_name in files:
                tmp.write(f'{file_name}\n')
            tmp_list = Path(tmp.name)
            tmp_json = tmp_list.with_suffix('.json')
        try:
            self.run('west', 'ncs-sbom', '--input-list-file', tmp_list, '--license-detectors',
                     'spdx-tag,full-text,external-file', '--output-cache-database', tmp_json)
            with open(tmp_json, encoding='utf-8') as fd:
                output = json.load(fd)
                return output['files']
        finally:
            unlink_quietly(tmp_list)
            unlink_quietly(tmp_json)

    def show_results(self, detected: 'list[dict]') -> bool:
        '''Interpret detected licenses and report results to the user.'''
        for file_name, file_info in detected.items():
            if len(file_info['license']) == 0:
                file_info['license'] = ['NONE']
            for license in file_info['license']:
                if self.license_checker.check(license, file_name):
                    message = LICENSE_ALLOWED if license != 'NONE' else NONE_LICENSE_ALLOWED
                    self.report('ok', message.replace('*', license), file_name, license)
                elif self.license_checker.check('-' + license, file_name):
                    message = LICENSE_WARNING if license != 'NONE' else NONE_LICENSE_WARNING
                    self.report('warning', message.replace('*', license), file_name, license)
                elif self.license_checker.check('-ANY', file_name):
                    self.report('warning', ANY_LICENSE_WARNING, file_name, license)
                else:
                    message = LICENSE_ERROR if license != 'NONE' else NONE_LICENSE_ERROR
                    self.report('error', message.replace('*', license), file_name, license)
        if self.total_errors > 0:
            print(f'License check failed with {self.total_errors} error(s) '
                  f'and {self.total_warnings} warning(s)!')
            print(RECOMMENDATIONS_TEXT)
            return False
        elif self.total_warnings > 0:
            print(f'License check successful, but with {self.total_warnings} warning(s)!')
            print(RECOMMENDATIONS_TEXT)
            return True
        else:
            print('License check successful.')
            return True

    def write_junit(self):
        '''Write the JUnit file.'''
        test_suite = junit_xml.TestSuite("LicenseCheck", self.junit_test_cases)
        with open(self.args.output, 'w') as fd:
            junit_xml.TestSuite.to_file(fd, [test_suite], prettyprint=False)

    def check(self) -> bool:
        '''Do the license check based on command-line arguments provided in the constructor.'''

        self.junit_test_cases = []
        self.total_tests = 0
        self.total_skipped = 0
        self.total_errors = 0
        self.total_warnings = 0
        self.git_top = Path(self.run('git', 'rev-parse', '--show-toplevel')).resolve()
        self.west_workspace = Path(self.git_top).parent

        print(f'Repository top directory: {self.git_top}')
        print(f'West workspace directory: {self.west_workspace}')

        files = self.generate_list_of_files()
        files += self.collect_manifest_files()
        files = self.skip_files(files)
        detected = self.detect_licenses(files) if files else dict()
        success = self.show_results(detected)
        self.write_junit()

        return success


def main():
    '''Main function.'''
    args = parse_args()
    checker = PatchLicenseChecker(args)
    success = checker.check()
    if not success:
        sys.exit(1)


if __name__ == '__main__':
    main()
