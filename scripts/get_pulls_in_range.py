#!/usr/bin/env python3

# Copyright (c) 2020-2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from contextlib import closing
from collections import defaultdict
import getpass
import netrc
import os
import sqlite3
import sys
from typing import Dict, List, NamedTuple, Optional, Union

import pygit2
import github

sys.path.append(os.path.join(os.path.dirname(__file__), 'west_commands'))
from pygit2_helpers import zephyr_commit_area, commit_title

# A container for pull request information.
class pr_info(NamedTuple):
    number: int
    title: str
    html_url: str
    commits: List[pygit2.Commit]

NO_PULL_REQUEST = -1

# A tuple describing the results of querying the file system and
# GitHub API for information about a commit.
#
# pr_num is either a positive pull request number or NO_PULL_REQUEST.
#
# NO_PULL_REQUEST happens when commits are directly pushed to a branch
# without a PR. In that case, pr_title and pr_url will be empty
# strings.
class commit_result(NamedTuple):
    sha: str
    remote_org: str
    remote_repo: str
    pr_num: int
    pr_title: str
    pr_url: str

def check_commit_result(result: commit_result):
    if result.pr_num == NO_PULL_REQUEST:
        assert result.pr_title == ''
        assert result.pr_url == ''

class ResultsDatabase:
    '''Object oriented interface for accessing the results database.

    Usage pseudocode:

        with ResultsDatabase('my-sqlite.db') as db:
            for query in my_query_list:
                previous_result = db.get_result(query.sha)
                if previous_result:
                    continue

                result = fetch_result(query)
                db.add_result(result)

    This pattern ensures that you don't call fetch_result() for data
    that are already available, and makes sure to checkpoint any results
    in the database, even if fetch_result() throws an exception.
    '''

    # No efforts have been made to optimize this interface.

    _CREATE_TABLE_IF_NOT_EXISTS = '''
    CREATE TABLE IF NOT EXISTS results
    (sha TEXT PRIMARY KEY NOT NULL,
    remote_org TEXT, remote_repo TEXT,
    pr_num INTEGER, pr_title TEXT, pr_url TEXT)
    '''

    def __init__(self, sqlite_db=None):
        self.sqlite_db = sqlite_db or ':memory:'
        self._con = None

    def __enter__(self):
        self._con = sqlite3.connect(self.sqlite_db)
        self._con.execute(self._CREATE_TABLE_IF_NOT_EXISTS)
        self._con.commit()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if self._con is not None:
            self._con.commit()
            self._con.close()
        self._con = None

    def get_result(self,
                   commit_or_sha: Union[str, pygit2.Commit]) -> commit_result:
        '''Get the commit_result from a previous run, or None.

        You can only call this during the time that this object is
        live within a with statement.'''
        if isinstance(commit_or_sha, pygit2.Commit):
            sha = str(commit_or_sha.oid)
        else:
            sha = commit_or_sha

        with closing(self._con.cursor()) as cursor:
            cursor.execute("SELECT * from results WHERE sha=?", (sha,))
            result = cursor.fetchone()
            if result is None:
                return None
            else:
                ret = commit_result(*tuple(result))
                check_commit_result(ret)
                return ret

    def add_result(self, result: commit_result) -> None:
        '''Insert a new commit_result into the database.

        Raises sqlite3.IntegrityError if duplicate entries for the
        same SHA are added to the database.
        '''
        check_commit_result(result)
        with closing(self._con.cursor()) as cursor:
            cursor.execute('INSERT INTO results VALUES '
                           '(?,?,?,?,?,?)', result)

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        epilog='''Get a list of pull requests associated
        with a range of commits in a GitHub repository.

        For promptless operation, either set up a ~/.netrc with
        credentials for github.com, or define a GITHUB_TOKEN environment
        variable.

        You are likely to run into problems with GitHub API rate limiting
        with larger commit ranges. To save and restore results from earlier
        runs, use the --sqlite-db option. This avoids requesting information
        from the GitHub API server that is already available locally.
        ''',
        allow_abbrev=False)

    parser.add_argument('gh_repo',
                        help='''repository in <organization>/<repo> format,
                        like zephyrproject-rtos/zephyr''')
    parser.add_argument('local_path',
                        help='path to repository on the file system')
    parser.add_argument('start', help='start commit ref in the range')
    parser.add_argument('end', help='end commit ref in the range')

    parser.add_argument('-z', '--zephyr-areas', action='store_true',
                        help='''local_path is zephyr; also try to figure
                        out what areas the pull requests affect''')
    parser.add_argument('-A', '--zephyr-only-areas', metavar='AREA',
                        action='append',
                        help='''local_path is zephyr; just output changes for
                        area AREA (may be given more than once)''')

    parser.add_argument('-d', '--sqlite-db',
                        help='''sqlite database file for storing results;
                        will be created if it does not exist''')

    ret = parser.parse_args()

    if ret.zephyr_only_areas:
        ret.zephyr_areas = True

    return ret

def get_gh_credentials() -> Dict:
    # Get github.Github credentials.
    #
    # This function tried to get github.com credentials from a
    # ~/.netrc file if it exists, and falls back on a GITHUB_TOKEN
    # environment variable if that does not work.
    #
    # If GITHUB_TOKEN is not set, we ask the user for the token
    # at the command line.
    #
    # We need credentials because access to api.github.com is
    # authenticated by GitHub for rate limiting purposes.
    #
    # The return value is a dict which can be passed to the Github
    # constructor as **kwargs.

    try:
        nrc = netrc.netrc()
    except FileNotFoundError:
        nrc = None
    except netrc.NetrcParseError:
        print('parse error in netrc file, falling back on environment',
              file=sys.stderr)
        nrc = None

    if nrc is not None:
        auth = nrc.authenticators('github.com')
        if auth is not None:
            return {'login_or_token': auth[0], 'password': auth[2]}

    token = os.environ.get('GITHUB_TOKEN')

    if not token:
        print('Missing GitHub credentials:\n'
              '~/.netrc file not found or has no github.com credentials, '
              'and GITHUB_TOKEN is not set in the environment. '
              'Please give your GitHub token.',
              file=sys.stderr)
        token = getpass.getpass('password/token: ')

    return {'login_or_token': token}

def get_gh_repo(repo_org_and_name) -> github.Repository:
    # Get the remote repository from the org/repo string in the
    # command line arguments.
    #
    # This requires getting github.com credentials.

    return github.Github(**get_gh_credentials()).get_repo(repo_org_and_name)

def get_pygit2_commits(args: argparse.Namespace) -> List[pygit2.Commit]:
    # Get commit objects for the commit range given by args.start and
    # args.end from the repository in args.local_path.

    repo = pygit2.Repository(args.local_path)
    walker = repo.walk(repo.revparse_single(args.end).oid)
    walker.hide(repo.revparse_single(args.start).oid)
    return list(walker)

def get_commit_results_for_range(
        repo_org_and_name: str,
        commit_list: List[pygit2.Commit],
        sqlite_db: Optional[str]) -> List[commit_result]:
    # Get information about pull requests which are associated with a
    # commit range.
    #
    # This can take a long time to run, so we print what we find to
    # stderr as we go, as a sign of life so users know what's
    # happening.
    #
    # It is also subject to network errors and GitHub API rate
    # limiting, so we cache results locally in args.sqlite_db. This
    # defaults to an in-memory database if the argument is None, allowing
    # easy getting started at the cost of throwing information away.

    results = {}  # sha to commit_result
    with ResultsDatabase(sqlite_db) as db:
        # Get pre-existing results from database file.
        if sqlite_db is not None:
            results.update(get_results_from_db(db, commit_list))

        # Fetch results which are still missing from the network.
        remaining = len(commit_list) - len(results)
        if remaining != 0:
            print(f'Fetching data for {remaining} commits from the network...',
                  file=sys.stderr)
            gh_repo = get_gh_repo(repo_org_and_name)
            for commit in commit_list:
                sha = str(commit.oid)
                if sha not in results:
                    results[sha] = get_result_from_network(commit, gh_repo, db)
            print('Done fetching commit data from the network.',
                  file=sys.stderr)

    # Give back the results in 'git log' order.
    return [results[str(commit.oid)] for commit in commit_list]

def get_results_from_db(
        db: ResultsDatabase,
        commit_list: List[pygit2.Commit]) -> Dict[str, commit_result]:
    # Fetches known results from db for the commits in commit_list.
    # Prints signs of life to stderr.

    results = {}
    for commit in commit_list:
        result = db.get_result(commit)
        if result is not None:
            results[result.sha] = result

    print(f'Found {len(results)} commit results in the database.',
          file=sys.stderr)
    return results

def get_result_from_network(commit: pygit2.Commit,
                            gh_repo: github.Repository,
                            db: ResultsDatabase) -> commit_result:
    # Fetch a commit_result from the network for 'commit'.
    # Saves the new commit_result in db before returning.
    # Prints signs of life to stderr.

    # Get the commit_result for commit from gh_repo.
    sha = str(commit.oid)
    gh_prs = list(gh_repo.get_commit(sha).get_pulls())
    if len(gh_prs) == 0:
        gh_pr = None
        result = commit_result(sha, gh_repo.owner.login, gh_repo.name,
                               NO_PULL_REQUEST, '', '')
    elif len(gh_prs) == 1:
        gh_pr = gh_prs[0]
        result = commit_result(sha, gh_repo.owner.login, gh_repo.name,
                               gh_pr.number, gh_pr.title, gh_pr.html_url)
    else:
        sys.exit(f'{sha} has {len(gh_prs)} prs (expected 1): {gh_prs}')

    # Cache the result in the database.
    db.add_result(result)

    # Print sign of life.
    if gh_pr:
        print(f'\t{sha[:10]} {commit_title(commit)}\n'
              f'\t           PR #{gh_pr.number:5}: {gh_pr.title}',
              file=sys.stderr)
    else:
        print(f'\t{sha[:10]} {commit_title(commit)}\n'
              f'\t           Not merged via pull request',
              file=sys.stderr)

    return result

def guess_pr_area(commits: List[pygit2.Commit]) -> str:
    # Assign an area to the PR by taking the area of each commit, and
    # picking the one that happens the most. Hopefully this is good
    # enough. We could consider adding pull request label tracking to
    # what we retrieve from GitHub if it turns out not to be.

    # area_counts maps areas to the number of commits with that area,
    # for the list of commits in 'commits'.
    area_counts = defaultdict(int)
    for commit in commits:
        area_counts[zephyr_commit_area(commit)] += 1

    # Try not to return 'Testing' as the result of this function if
    # multiple areas have the maximum count in 'area_counts'.
    #
    # This avoids situations like having a PR with one commit with a
    # new feature, another commit with tests, and assigning the whole
    # PR to "Testing".
    max_count = max(area_counts.values())
    max_count_areas = [area for area, count in area_counts.items()
                       if count == max_count]

    for area in max_count_areas:
        if area != 'Testing':
            return area

    return max_count_areas[0]

def print_pr_info(info):
    print(f'- #{info.number}: {info.title}\n'
          f'  {info.html_url}')

def main():
    args = parse_args()

    if args.sqlite_db is None:
        print('warning: --sqlite-db was not given; you will need to '
              'start over if you are rate-limited or another error occurs',
              file=sys.stderr)

    commit_list = get_pygit2_commits(args)
    sha_to_commit = {str(commit.oid): commit for commit in commit_list}
    print(f'Range {args.start}..{args.end} has {len(commit_list)} commits.',
          file=sys.stderr)

    # Get the results.
    results = get_commit_results_for_range(args.gh_repo, commit_list,
                                           args.sqlite_db)

    # ResultsDatabase representations are commit-centric. Convert
    # these to a pull-request-centric representation.
    pr_num_to_commits = defaultdict(list)
    pr_num_to_results = defaultdict(list)
    for result in results:
        pr_num_to_commits[result.pr_num].append(sha_to_commit[result.sha])
        pr_num_to_results[result.pr_num].append(result)
    pr_num_to_info = {}
    for pr_num, commits in pr_num_to_commits.items():
        results = pr_num_to_results[pr_num]
        pr_num_to_info[pr_num] = pr_info(pr_num,
                                         results[0].pr_title,
                                         results[0].pr_url,
                                         commits)

    no_pr_commits = pr_num_to_commits.pop(NO_PULL_REQUEST, [])
    if no_pr_commits:
        del pr_num_to_results[NO_PULL_REQUEST]
        del pr_num_to_info[NO_PULL_REQUEST]

    # Print information about each resulting pull request.
    if args.zephyr_areas:
        area_to_pr_infos = defaultdict(list)
        for pr_num, info in pr_num_to_info.items():
            assert pr_num != NO_PULL_REQUEST
            area = guess_pr_area(info.commits)
            area_to_pr_infos[area].append(info)

        print('Summary\n'
              '-------\n\n'
              f'{len(pr_num_to_info)} pull requests found in the range.\n')
        for area in sorted(area_to_pr_infos):
            if args.zephyr_only_areas and area not in args.zephyr_only_areas:
                continue
            print(f'- {area}: {len(area_to_pr_infos[area])} pull requests')

        for area in sorted(area_to_pr_infos):
            if args.zephyr_only_areas and area not in args.zephyr_only_areas:
                continue
            infos = area_to_pr_infos[area]
            title = f'{area} ({len(infos)} pull requests)'
            print(f'\n{title}')
            print('-' * len(title))
            print()
            for info in infos:
                print_pr_info(info)
    else:
        for info in pr_num_to_info.values():
            print_pr_info(info)

    if no_pr_commits:
        print('\nCommits not merged via pull request:\n')
        for commit in no_pr_commits:
            print(f'- {commit.oid} {commit_title(commit)}')

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
