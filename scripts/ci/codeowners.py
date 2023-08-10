#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

# Copyright (c) 2018,2020 Intel Corporation
# Copyright (c) 2022 Nordic Semiconductor ASA

import argparse
import collections
import logging
import os
from pathlib import Path
import re
import subprocess
import sys
import shlex

logger = None

failures = 0

def err(msg):
    cmd = sys.argv[0]  # Empty if missing
    if cmd:
        cmd += ": "
    sys.exit(f"{cmd}fatal error: {msg}")


def cmd2str(cmd):
    # Formats the command-line arguments in the iterable 'cmd' into a string,
    # for error messages and the like

    return " ".join(shlex.quote(word) for word in cmd)


def annotate(severity, file, title, message, line=None, col=None):
    """
    https://docs.github.com/en/actions/using-workflows/workflow-commands-for-github-actions#about-workflow-commands
    """
    notice = f'::{severity} file={file}' + \
             (f',line={line}' if line else '') + \
             (f',col={col}' if col else '') + \
             f',title={title}::{message}'
    print(notice)


def failure(msg, file='CODEOWNERS', line=None):
    global failures
    failures += 1
    annotate('error', file=file, title="CODEOWNERS", message=msg,
             line=line)


def git(*args, cwd=None):
    # Helper for running a Git command. Returns the rstrip()ed stdout output.
    # Called like git("diff"). Exits with SystemError (raised by sys.exit()) on
    # errors. 'cwd' is the working directory to use (default: current
    # directory).

    git_cmd = ("git",) + args
    try:
        cp = subprocess.run(git_cmd, capture_output=True, cwd=cwd)
    except OSError as e:
        err(f"failed to run '{cmd2str(git_cmd)}': {e}")

    if cp.returncode or cp.stderr:
        err(f"'{cmd2str(git_cmd)}' exited with status {cp.returncode} and/or "
            f"wrote to stderr.\n"
            f"==stdout==\n"
            f"{cp.stdout.decode('utf-8')}\n"
            f"==stderr==\n"
            f"{cp.stderr.decode('utf-8')}\n")

    return cp.stdout.decode("utf-8").rstrip()


def get_files(filter=None, paths=None):
    filter_arg = (f'--diff-filter={filter}',) if filter else ()
    paths_arg = ('--', *paths) if paths else ()
    return git('diff', '--name-only', *filter_arg, COMMIT_RANGE, *paths_arg)


def ls_owned_files(codeowners):
    """Returns an OrderedDict mapping git patterns from the CODEOWNERS file
    to the corresponding list of files found on the filesystem.  It
    unfortunately does not seem possible to invoke git and re-use
    how 'git ignore' and/or 'git attributes' already implement this,
    we must re-invent it.
    """

    # TODO: filter out files not in "git ls-files" (e.g.,
    # twister-out) _if_ the overhead isn't too high for a clean tree.
    #
    # pathlib.match() doesn't support **, so it looks like we can't
    # recursively glob the output of ls-files directly, only real
    # files :-(

    pattern2files = collections.OrderedDict()
    top_path = Path(GIT_TOP)

    with open(codeowners, "r") as codeo:
        for lineno, line in enumerate(codeo, start=1):

            if line.startswith("#") or not line.strip():
                continue

            match = re.match(r"^([^\s,]+)\s+[^\s]+", line)
            if not match:
                failure(f"Invalid CODEOWNERS line {lineno}\n\t{line}",
                        file='CODEOWNERS', line=lineno)
                continue

            git_patrn = match.group(1)
            glob = git_pattern_to_glob(git_patrn)
            files = []
            for abs_path in top_path.glob(glob):
                # comparing strings is much faster later
                files.append(str(abs_path.relative_to(top_path)))

            if not files:
                failure(f"Path '{git_patrn}' not found in the tree"
                             f"but is listed in CODEOWNERS")

            pattern2files[git_patrn] = files

    return pattern2files


def git_pattern_to_glob(git_pattern):
    """Appends and prepends '**[/*]' when needed. Result has neither a
    leading nor a trailing slash.
    """

    if git_pattern.startswith("/"):
        ret = git_pattern[1:]
    else:
        ret = "**/" + git_pattern

    if git_pattern.endswith("/"):
        ret = ret + "**/*"
    elif os.path.isdir(os.path.join(GIT_TOP, ret)):
        failure("Expected '/' after directory '{}' "
                         "in CODEOWNERS".format(ret))

    return ret


def codeowners():
    codeowners = os.path.join(GIT_TOP, "CODEOWNERS")
    if not os.path.exists(codeowners):
        err("CODEOWNERS not available in this repo")

    name_changes = get_files(filter="ARCD")
    owners_changes = get_files(paths=(codeowners,))

    if not name_changes and not owners_changes:
        # TODO: 1. decouple basic and cheap CODEOWNERS syntax
        # validation from the expensive ls_owned_files() scanning of
        # the entire tree. 2. run the former always.
        return

    logger.info("If this takes too long then cleanup and try again")
    patrn2files = ls_owned_files(codeowners)

    # The way git finds Renames and Copies is not "exact science",
    # however if one is missed then it will always be reported as an
    # Addition instead.
    new_files = get_files(filter="ARC").splitlines()
    logger.debug(f"New files {new_files}")

    # Convert to pathlib.Path string representation (e.g.,
    # backslashes 'dir1\dir2\' on Windows) to be consistent
    # with ls_owned_files()
    new_files = [str(Path(f)) for f in new_files]

    new_not_owned = []
    for newf in new_files:
        f_is_owned = False

        for git_pat, owned in patrn2files.items():
            logger.debug(f"Scanning {git_pat} for {newf}")

            if newf in owned:
                logger.info(f"{git_pat} matches new file {newf}")
                f_is_owned = True
                # Unlike github, we don't care about finding any
                # more specific owner.
                break

        if not f_is_owned:
            new_not_owned.append(newf)

    if new_not_owned:
        failure("New files added that are not covered in "
                     "CODEOWNERS:\n\n" + "\n".join(new_not_owned) +
                     "\n\nPlease add one or more entries in the "
                     "CODEOWNERS file to cover those files")


def init_logs(cli_arg):
    # Initializes logging

    global logger

    level = os.environ.get('LOG_LEVEL', "WARN")

    console = logging.StreamHandler()
    console.setFormatter(logging.Formatter('%(levelname)-8s: %(message)s'))

    logger = logging.getLogger('')
    logger.addHandler(console)
    logger.setLevel(cli_arg or level)

    logger.info("Log init completed, level=%s",
                 logging.getLevelName(logger.getEffectiveLevel()))


def parse_args():
    default_range = 'HEAD~1..HEAD'
    parser = argparse.ArgumentParser(
        description="Check for CODEOWNERS file ownership.")
    parser.add_argument('-c', '--commits', default=default_range,
                        help=f'''Commit range in the form: a..[b], default is
                        {default_range}''')
    parser.add_argument("-v", "--loglevel", choices=['DEBUG', 'INFO', 'WARNING',
                                                     'ERROR', 'CRITICAL'],
                        help="python logging level")

    return parser.parse_args()


def main():
    args = parse_args()

    # The absolute path of the top-level git directory. Initialize it here so
    # that issues running Git can be reported to GitHub.
    global GIT_TOP
    GIT_TOP = git("rev-parse", "--show-toplevel")

    # The commit range passed in --commit, e.g. "HEAD~3"
    global COMMIT_RANGE
    COMMIT_RANGE = args.commits

    init_logs(args.loglevel)
    logger.info(f'Running tests on commit range {COMMIT_RANGE}')

    codeowners()

    sys.exit(failures)


if __name__ == "__main__":
    main()
