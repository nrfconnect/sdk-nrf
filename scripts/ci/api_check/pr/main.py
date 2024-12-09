# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import os
import re
import sys
import json
from pathlib import Path
from github import Github
from types import SimpleNamespace
from jinja2 import Template
from github.Repository import Repository
from github.PullRequest import PullRequest
from github.IssueComment import IssueComment
from github.WorkflowRun import WorkflowRun


API_CHECK_COMMENT_INDICATOR = '<!-- API-check comment -->'


class TemplateData(SimpleNamespace):
    notice: int
    warning: int
    critical: int
    github_actor: str
    repo: Repository
    pr: PullRequest
    run: WorkflowRun
    def __init__(self, file: os.PathLike):
        with open(file, 'r') as fd:
            dict = json.load(fd)
        super().__init__(**dict)

def fatal(*args, **kwargs):
    print(*args, **kwargs, file=sys.stderr)
    sys.exit(1)

def get_stats() -> TemplateData:
    stats: 'TemplateData | None' = None
    for arg in sys.argv[1:]:
        if not Path(arg).exists():
            fatal(f'The "{arg}" does not exist. Probably checking script failed.')
        file_stats = TemplateData(arg)
        if stats:
            stats.notice += file_stats.notice
            stats.warning += file_stats.warning
            stats.critical += file_stats.critical
        else:
            stats = file_stats
    if stats is None:
        fatal('No input files.')
    return stats

def get_message(data: TemplateData) -> str:
    template_path: Path = Path(__file__).parent / 'pr-comment.md.jinja'
    template = Template(template_path.read_text())
    message = API_CHECK_COMMENT_INDICATOR + '\n' + template.render(**data.__dict__).strip()
    return message

def get_meta(message, keyword) -> list[str]:
    result = []
    for match in re.finditer(r'<!--\s*' + keyword + r':\s*(.*?)\s*-->', message, re.DOTALL):
        result.append(match.group(1))
    return result

def main():
    data = get_stats()
    print('Stats', data)

    github = Github(os.environ['GITHUB_TOKEN'])
    print(f'Github API connected. Remaining requests {github.rate_limiting[0]} of {github.rate_limiting[1]}.')

    data.github_actor = os.environ['GITHUB_ACTOR']
    print(f'Github user: {data.github_actor}')

    data.repo = github.get_repo(os.environ['GITHUB_REPO'], lazy=True)
    data.pr = data.repo.get_pull(int(os.environ['PR_NUMBER']))
    print(f'Pull request: {data.pr.title} #{data.pr.number} {data.pr.html_url}')

    data.run = data.repo.get_workflow_run(int(os.environ['GITHUB_RUN_ID']))
    print(f'Workflow run: {data.run.id}')

    message = get_message(data)
    print(f'Comment message:\n{message}\n------------------------------------')

    comment: IssueComment | None
    for comment in data.pr.get_issue_comments():
        if comment.body.strip().startswith(API_CHECK_COMMENT_INDICATOR):
            if message == comment.body:
                print(f'Comment unchanged: {comment.html_url}')
            else:
                print(f'Editing comment: {comment.html_url}')
                comment.edit(message)
            break
    else:
        print(f'Adding new comment.')
        comment = data.pr.create_issue_comment(message)
        print(f'Added comment: {comment.html_url}')

    labels = get_meta(message, 'add-label')
    if len(labels) > 0:
        print(f'Adding labels: {", ".join(labels)}')
        data.pr.add_to_labels(*labels)

    for label in get_meta(message, 'remove-label'):
        print(f'Removing label: {label}')
        for existing_label in data.pr.labels:
            if existing_label.name == label:
                data.pr.remove_from_labels(label)
                break
        else:
            print(f'Label already removed: {label}')

    exit_code = 0
    for value in get_meta(message, 'exit-code'):
        exit_code = int(value)
    sys.exit(exit_code)


if __name__ == '__main__':
    main()
