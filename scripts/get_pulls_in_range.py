#!/usr/bin/env python3

import argparse
from collections import defaultdict, namedtuple
import getpass
import netrc
import os
import sys

import pygit2
from github import Github

from pygit2_helpers import zephyr_commit_area, commit_shortlog

# A container for pull request information. The 'pull_request'
# attribute is a github.PullRequest, and 'commits' is a list of
# pygit2.Commit objects.
#
# If args.zephyr_areas is given, zephyr_area is not None and is a
# string determined by heuristic for what area the PR touched most,
# based on its commit shortlogs.
pr_info = namedtuple('pr_info', 'pull_request commits zephyr_area')

def parse_args():
    parser = argparse.ArgumentParser(
        epilog='''Get a list of pull requests associated
        with a range of commits in a GitHub repository.

        For promptless operation, either set up a ~/.netrc with
        credentials for github.com, or define a GITHUB_TOKEN environment
        variable.''')

    parser.add_argument('gh_repo',
                        help='''repository in <organization>/<repo> format,
                        like nrfconnect/sdk-nrf''')
    parser.add_argument('local_path',
                        help='path to repository on the file system')
    parser.add_argument('start', help='start commit ref in the range')
    parser.add_argument('end', help='end commit ref in the range')

    parser.add_argument('-z', '--zephyr-areas', action='store_true',
                        help='''local_path is zephyr; also try to figure
                        out what areas the pull requests affect''')

    return parser.parse_args()

def get_gh_credentials():
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

def get_gh_repo(args):
    # Get a github.Repository object for the remote repository from
    # the org/repo string in the command line arguments. This requires
    # getting github.com credentials.

    return Github(**get_gh_credentials()).get_repo(args.gh_repo)

def get_pygit2_commits(args):
    # Get a list of pygit2.Commit objects for the commit range given
    # by the command line arguments from the repository on the local
    # file system.

    repo = pygit2.Repository(args.local_path)
    walker = repo.walk(repo.revparse_single(args.end).oid)
    walker.hide(repo.revparse_single(args.start).oid)
    return list(walker)

def get_pull_info_in_range(args):
    # Get information about pull requests which are associated with a
    # commit range.
    #
    # This can take a long time to run, so we print what we find to
    # stderr as we go, as a sign of life so users know what's
    # happening.
    #
    # The return value is a list of pr_info tuples.

    # Get list of commits from local repository, as pygit2.Commit objects.
    commit_list = get_pygit2_commits(args)

    # Get associated PRs using GitHub API, as github.PullRequest objects.
    # Map each pull request number to its object and commits.
    gh_repo = get_gh_repo(args)
    pr_num_to_pr = {}
    pr_num_to_commits = defaultdict(list)
    if args.zephyr_areas:
        sha_to_area = {}
    for commit in commit_list:
        sha = str(commit.oid)
        if args.zephyr_areas:
            area = zephyr_commit_area(commit)
            sha_to_area[sha] = area
            area_str = f'{area:13}:'
        else:
            area_str = ''

        gh_prs = list(gh_repo.get_commit(sha).get_pulls())
        if len(gh_prs) != 1:
            sys.exit(f'{sha} has {len(gh_prs)} prs (expected 1): {gh_prs}')
        gh_pr = gh_prs[0]
        pr_num_to_commits[gh_pr.number].append(commit)
        pr_num_to_pr[gh_pr.number] = gh_pr
        # cut off the shortlog to a fixed length for readability of the output.
        shortlog = commit_shortlog(commit)[:15]
        print(f'{sha}:{area_str}{shortlog:15}:{gh_pr.html_url}:{gh_pr.title}',
              file=sys.stderr)
    print(file=sys.stderr)

    # Bundle up the return dict
    ret = []
    for pr_num, commits in pr_num_to_commits.items():
        if args.zephyr_areas:
            # Assign an area to the PR by taking the area of each
            # commit, and picking the one that happens the most.
            # Hopefully this is good enough.
            area_counts = defaultdict(int)
            for commit in commits:
                sha = str(commit.oid)
                area_counts[sha_to_area[sha]] += 1
            zephyr_area = max(area_counts, key=lambda area: area_counts[area])
        else:
            zephyr_area = None

        ret.append(pr_info(pr_num_to_pr[pr_num], commits, zephyr_area))

    return ret

def print_pr(pr):
    print(f'- #{pr.number}: {pr.title}\n'
          f'  {pr.html_url}')

def main():
    args = parse_args()

    # Get the pull request info.
    pr_info_list = get_pull_info_in_range(args)

    print(f'{len(pr_info_list)} pull requests total were found.\n')
    if args.zephyr_areas:
        area_to_prs = defaultdict(list)
        for info in pr_info_list:
            area_to_prs[info.zephyr_area].append(info.pull_request)
        print('Pull requests grouped by a guess of the zephyr area:')
        for area, prs in area_to_prs.items():
            print(f'\n{area}')
            print('-' * len(area))
            print()
            for pr in prs:
                print_pr(pr)
    else:
        for info in pr_info_list:
            print_pr(info.pull_request)

if __name__ == '__main__':
    main()
