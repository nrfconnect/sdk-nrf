#!/usr/bin/env python3
#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import argparse
import json
import re
import sys
import shlex
import subprocess
import tempfile
import textwrap
from pathlib import Path
import yaml
import junit_xml


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
    return parser.parse_args()


def unlink_quietly(path: Path) -> None:
    '''Delete a file if it exists.'''
    try:
        path.unlink()
    except FileNotFoundError:
        pass


class FileLicenseChecker:
    '''Class that checks if a license is allowed for a file.'''

    allow_list: 'dict[str, list[tuple[re.Pattern, bool]]]'

    def __init__(self, allow_list_file: Path):
        with open(allow_list_file, 'r') as fd:
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
    '''Check licenses for a git patch.'''

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
                    file_path = (self.west_workspace / file_name).relative_to(self.git_top)
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
            with open(tmp_json, 'r', encoding='utf-8') as fd:
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
        files = self.skip_files(files)
        detected = self.detect_licenses(files) if files else []
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
