#!/usr/bin/env python3

# Copyright (c) 2018,2020 Intel Corporation
# Copyright (c) 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

import argparse
import contextlib
import logging
import os
import shlex
import subprocess
import sys
import traceback
from pathlib import Path

from junitparser import Error, Failure, JUnitXml, Skipped, TestCase, TestSuite

with contextlib.suppress(ImportError):
    pass

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "zephyr" / "scripts"))

# Let the user run this script as ./scripts/ci/check_compliance.py without
# making them set ZEPHYR_BASE.
ZEPHYR_BASE = os.environ.get('ZEPHYR_BASE')
if ZEPHYR_BASE:
    ZEPHYR_BASE = Path(ZEPHYR_BASE)
else:
    ZEPHYR_BASE = Path(__file__).resolve().parents[2]
    # Propagate this decision to child processes.
    os.environ['ZEPHYR_BASE'] = str(ZEPHYR_BASE)

logger = None


def git(*args, cwd=None, ignore_non_zero=False):
    # Helper for running a Git command. Returns the rstrip()ed stdout output.
    # Called like git("diff"). Exits with SystemError (raised by sys.exit()) on
    # errors if 'ignore_non_zero' is set to False (default: False). 'cwd' is the
    # working directory to use (default: current directory).

    git_cmd = ("git",) + args
    try:
        cp = subprocess.run(git_cmd, capture_output=True, cwd=cwd)
    except OSError as e:
        err(f"failed to run '{cmd2str(git_cmd)}': {e}")

    if not ignore_non_zero and (cp.returncode or cp.stderr):
        err(
            f"'{cmd2str(git_cmd)}' exited with status {cp.returncode} and/or "
            f"wrote to stderr.\n"
            f"==stdout==\n"
            f"{cp.stdout.decode('utf-8')}\n"
            f"==stderr==\n"
            f"{cp.stderr.decode('utf-8')}\n"
        )

    return cp.stdout.decode("utf-8").rstrip()


def get_shas(refspec):
    """
    Returns the list of Git SHAs for 'refspec'.

    :param refspec:
    :return:
    """
    return git('rev-list', f'--max-count={-1 if "." in refspec else 1}', refspec).split()


def get_files(filter=None, paths=None):
    filter_arg = (f'--diff-filter={filter}',) if filter else ()
    paths_arg = ('--', *paths) if paths else ()
    out = git('diff', '--name-only', *filter_arg, COMMIT_RANGE, *paths_arg)
    files = out.splitlines()
    for file in list(files):
        if not (GIT_TOP / file).exists():
            # Drop submodule directories from the list.
            files.remove(file)
    return files


class FmtdFailure(Failure):
    def __init__(
        self, severity, title, file, line=None, col=None, desc="", end_line=None, end_col=None
    ):
        self.severity = severity
        self.title = title
        self.file = file
        self.line = line
        self.col = col
        self.end_line = end_line
        self.end_col = end_col
        self.desc = desc
        description = f':{desc}' if desc else ''
        msg_body = desc or title

        txt = (
            f'\n{title}{description}\nFile:{file}'
            + (f'\nLine:{line}' if line else '')
            + (f'\nColumn:{col}' if col else '')
            + (f'\nEndLine:{end_line}' if end_line else '')
            + (f'\nEndColumn:{end_col}' if end_col else '')
        )
        msg = f'{file}' + (f':{line}' if line else '') + f' {msg_body}'
        typ = severity.lower()

        super().__init__(msg, typ)

        self.text = txt


class ComplianceTest:
    """
    Base class for tests. Inheriting classes should have a run() method and set
    these class variables:

    name:
      Test name

    doc:
      Link to documentation related to what's being tested

    path_hint:
      The path the test runs itself in. By default it uses the magic string
      "<git-top>" which refers to the top-level repository directory.

      This avoids running 'git' to find the top-level directory before main()
      runs (class variable assignments run when the 'class ...' statement
      runs). That avoids swallowing errors, because main() reports them to
      GitHub.

      Subclasses may override the default with a specific path or one of the
      magic strings below:
      - "<zephyr-base>" can be used to refer to the environment variable
        ZEPHYR_BASE or, when missing, the calculated base of the zephyr tree.
    """

    path_hint = "<git-top>"

    def __init__(self):
        self.case = TestCase(type(self).name, "Guidelines")
        # This is necessary because Failure can be subclassed, but since it is
        # always restored form the element tree, the subclass is lost upon
        # restoring
        self.fmtd_failures = []

    def _result(self, res, text):
        res.text = text.rstrip()
        self.case.append(res)

    def error(self, text, msg=None, type_="error"):
        """
        Signals a problem with running the test, with message 'msg'.

        Raises an exception internally, so you do not need to put a 'return'
        after error().
        """
        err = Error(msg or f'{type(self).name} error', type_)
        self._result(err, text)

        raise EndTest

    def skip(self, text, msg=None, type_="skip"):
        """
        Signals that the test should be skipped, with message 'msg'.

        Raises an exception internally, so you do not need to put a 'return'
        after skip().
        """
        skpd = Skipped(msg or f'{type(self).name} skipped', type_)
        self._result(skpd, text)

        raise EndTest

    def failure(self, text, msg=None, type_="failure"):
        """
        Signals that the test failed, with message 'msg'. Can be called many
        times within the same test to report multiple failures.
        """
        fail = Failure(msg or f'{type(self).name} issues', type_)
        self._result(fail, text)

    def fmtd_failure(
        self, severity, title, file, line=None, col=None, desc="", end_line=None, end_col=None
    ):
        """
        Signals that the test failed, and store the information in a formatted
        standardized manner. Can be called many times within the same test to
        report multiple failures.
        """
        fail = FmtdFailure(severity, title, file, line, col, desc, end_line, end_col)
        self._result(fail, fail.text)
        self.fmtd_failures.append(fail)


class EndTest(Exception):
    """
    Raised by ComplianceTest.error()/skip() to end the test.

    Tests can raise EndTest themselves to immediately end the test, e.g. from
    within a nested function call.
    """


class WestLinkCheck(ComplianceTest):
    """
    West link synchronization check
    """

    name = "WestLink"
    doc = "Check that west link files are in sync."

    def run(self):
        cmd = [
            "west",
            "ln",
            "--status",
            COMMIT_RANGE,
        ]
        try:
            result = subprocess.run(cmd, check=False, capture_output=True, cwd=GIT_TOP)
        except Exception as ex:
            self.error(f"Failed to run west ln --status: {ex}")
        output_err = result.stderr.decode("utf-8")
        output_out = result.stdout.decode("utf-8")
        for line in output_out.splitlines()[1:]:
            state, line = line.split(":")
            if state in ('!', '-', '?'):
                self.failure(output_out)
                break

        if len(output_err) > 1:
            self.failure(output_err)


def init_logs(cli_arg):
    # Initializes logging

    global logger

    level = os.environ.get('LOG_LEVEL', "WARN")

    console = logging.StreamHandler()
    console.setFormatter(logging.Formatter('%(levelname)-8s: %(message)s'))

    logger = logging.getLogger('')
    logger.addHandler(console)
    logger.setLevel(cli_arg or level)

    logger.info("Log init completed, level=%s", logging.getLevelName(logger.getEffectiveLevel()))


def inheritors(klass):
    subclasses = set()
    work = [klass]
    while work:
        parent = work.pop()
        for child in parent.__subclasses__():
            if child not in subclasses:
                subclasses.add(child)
                work.append(child)
    return subclasses


def annotate(res, doc=None):
    """
    https://docs.github.com/en/actions/using-workflows/workflow-commands-for-github-actions#about-workflow-commands
    """

    def _esc(msg: str) -> str:
        return msg.replace('%', '%25').replace('\n', '%0A').replace('\r', '%0D')

    msg = _esc(res.message)
    if doc:
        msg += '%0A' + _esc(doc)

    notice = (
        f'::{res.severity} file={res.file}'
        + (f',line={res.line}' if res.line else '')
        + (f',col={res.col}' if res.col else '')
        + (f',endLine={res.end_line}' if res.end_line else '')
        + (f',endColumn={res.end_col}' if res.end_col else '')
        + f',title={res.title}::{msg}'
    )
    print(notice)


def resolve_path_hint(hint):
    if hint == "<zephyr-base>":
        return ZEPHYR_BASE
    elif hint == "<git-top>":
        return GIT_TOP
    else:
        return hint


def parse_args(argv):
    default_range = 'HEAD~1..HEAD'
    # Git root empty tree sha1 (represents a tree with no files)
    empty_tree = '4b825dc642cb6eb9a060e54bf8d69288fbee4904'
    parser = argparse.ArgumentParser(
        description="Check for coding style and documentation warnings.", allow_abbrev=False
    )
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        '-c',
        '--commits',
        default=default_range,
        help=f'''Commit range in the form: a..[b], default is
                        {default_range}''',
    )
    group.add_argument(
        '--all-commits',
        action='store_const',
        dest='commits',
        const=f'{empty_tree}..HEAD',
        help="""The full history commit range. Useful for testing purposes.
                WARNING: Should not be set for checks that perform per-commit actions, such as
                GitDiffCheck/GitLint/Identity.""",
    )
    parser.add_argument(
        '-o',
        '--output',
        default="ncs_compliance.xml",
        help='''Name of outfile in JUnit format,
                        default is ./compliance.xml''',
    )
    parser.add_argument(
        '-n',
        '--no-case-output',
        action="store_true",
        help="Do not store the individual test case output.",
    )
    parser.add_argument('-l', '--list', action="store_true", help="List all checks and exit")
    parser.add_argument(
        "-v",
        "--loglevel",
        choices=['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'],
        help="python logging level",
    )
    parser.add_argument(
        '-m',
        '--module',
        action="append",
        default=[],
        help="Checks to run. All checks by default. (case insensitive)",
    )
    parser.add_argument(
        '-e',
        '--exclude-module',
        action="append",
        default=[],
        help="Do not run the specified checks (case insensitive)",
    )
    parser.add_argument(
        '-j',
        '--previous-run',
        default=None,
        help='''Pre-load JUnit results in XML format
                        from a previous run and combine with new results.''',
    )
    parser.add_argument(
        '--annotate', action="store_true", help="Print GitHub Actions-compatible annotations."
    )

    return parser.parse_args(argv)


def _main(args):
    # The "real" main(), which is wrapped to catch exceptions and report them
    # to GitHub. Returns the number of test failures.

    # The absolute path of the top-level git directory. Initialize it here so
    # that issues running Git can be reported to GitHub.
    global GIT_TOP
    GIT_TOP = Path(git("rev-parse", "--show-toplevel"))

    # The commit range passed in --commit, e.g. "HEAD~3"
    global COMMIT_RANGE
    COMMIT_RANGE = args.commits

    init_logs(args.loglevel)

    if args.list:
        for testcase in sorted(inheritors(ComplianceTest), key=lambda x: x.name):
            print(testcase.name)
        return 0

    # Load saved test results from an earlier run, if requested
    if args.previous_run:
        if not os.path.exists(args.previous_run):
            # This probably means that an earlier pass had an internal error
            # (the script is currently run multiple times by the ci-pipelines
            # repo). Since that earlier pass might've posted an error to
            # GitHub, avoid generating a GitHub comment here, by avoiding
            # sys.exit() (which gets caught in main()).
            print(f"error: '{args.previous_run}' not found", file=sys.stderr)
            return 1

        logging.info(f"Loading previous results from {args.previous_run}")
        for loaded_suite in JUnitXml.fromfile(args.previous_run):
            suite = loaded_suite
            break
    else:
        suite = TestSuite("Compliance")

    included = list(map(lambda x: x.lower(), args.module))
    excluded = list(map(lambda x: x.lower(), args.exclude_module))

    for testcase in inheritors(ComplianceTest):
        # "Modules" and "testcases" are the same thing. Better flags would have
        # been --tests and --exclude-tests or the like, but it's awkward to
        # change now.

        if included and testcase.name.lower() not in included:
            continue

        if testcase.name.lower() in excluded:
            print("Skipping " + testcase.name)
            continue

        test = testcase()
        try:
            print(f"Running {test.name:30} tests in {resolve_path_hint(test.path_hint)} ...")
            test.run()
        except EndTest:
            pass
        except BaseException:
            test.failure(f"An exception occurred in {test.name}:\n{traceback.format_exc()}")

        # Annotate if required
        if args.annotate:
            for res in test.fmtd_failures:
                annotate(res, test.doc)

        suite.add_testcase(test.case)

    if args.output:
        xml = JUnitXml()
        xml.add_testsuite(suite)
        xml.update_statistics()
        xml.write(args.output, pretty=True)

    failed_cases = []
    warning_cases = []
    name2doc = {testcase.name: testcase.doc for testcase in inheritors(ComplianceTest)}

    for case in suite:
        if case.result:
            if case.is_skipped:
                logging.warning(f"Skipped {case.name}")
            else:
                if any(res.type in ('error', 'failure') for res in case.result):
                    failed_cases.append(case)
                else:
                    warning_cases.append(case)
        else:
            # Some checks can produce no .result
            logging.info(f"No JUnit result for {case.name}")

    n_fails = len(failed_cases)
    n_warnings = len(warning_cases)

    if n_fails or n_warnings:
        if n_fails:
            print(f"{n_fails} check(s) failed")
        if n_warnings:
            print(f"{n_warnings} check(s) with warnings only")

        for case in failed_cases + warning_cases:
            for res in case.result:
                errmsg = res.text.strip()
                if res.type in ('error', 'failure'):
                    logging.error(f"Test {case.name} failed: \n{errmsg}")
                else:
                    logging.warning(f"Test {case.name} warning: \n{errmsg}")

            if args.no_case_output:
                continue
            with open(f"{case.name}.txt", "w") as f:
                docs = name2doc.get(case.name)
                f.write(f"{docs}\n")
                for res in case.result:
                    errmsg = res.text.strip()
                    f.write(f'\n {errmsg}')

    if args.output:
        print(f"\nComplete results in {args.output}")
    return n_fails


def main(argv=None):
    args = parse_args(argv)

    try:
        n_fails = _main(args)
    except BaseException:
        # Catch BaseException instead of Exception to include stuff like
        # SystemExit (raised by sys.exit())
        print(f"Python exception in `{__file__}`:\n\n```\n{traceback.format_exc()}\n```")

        raise

    sys.exit(n_fails)


def cmd2str(cmd):
    # Formats the command-line arguments in the iterable 'cmd' into a string,
    # for error messages and the like

    return " ".join(shlex.quote(word) for word in cmd)


def err(msg):
    cmd = sys.argv[0]  # Empty if missing
    if cmd:
        cmd += ": "
    sys.exit(f"{cmd} error: {msg}")


if __name__ == "__main__":
    main(sys.argv[1:])
