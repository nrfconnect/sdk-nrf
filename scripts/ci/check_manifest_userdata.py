#!/usr/bin/env python3

from pathlib import Path
from pprint import pprint
import logging
import sys

from west.manifest import Manifest
import pykwalify.core

HERE = Path(__file__).parent
SCRIPTS = HERE.parent
SCHEMA = str(SCRIPTS / 'manifest-userdata-schema.yml')

def userdata_error_message(project):
    userdata = project.userdata

    if not (isinstance(userdata, dict) and 'ncs' in userdata):
        return None

    try:
        pykwalify.core.Core(source_data=userdata,
                            schema_files=[SCHEMA]).validate()
    except pykwalify.errors.SchemaError as se:
        return se.msg
    else:
        return None

def get_upstream_sha(project):
    if not project.userdata:
        return None

    if not 'ncs' in project.userdata:
        return None

    return project.userdata['ncs']['upstream-sha']

def main():
    # Silence validation errors from pykwalify, which are logged at
    # logging.ERROR level. We want to handle those ourselves as
    # needed.
    logging.getLogger('pykwalify').setLevel(logging.CRITICAL)

    manifest = Manifest.from_topdir()
    errors = False
    for project in manifest.projects:
        print(f'checking {project.name} userdata... ', end='')
        sys.stdout.flush()

        if project.name == 'manifest':
            if project.userdata:
                print('error: there should be no self: userdata: ', end='')
                pprint(project.userdata)
                print('To fix, remove this value.')
                errors = True
            else:
                print('OK')
            continue

        error_message = userdata_error_message(project)
        if error_message:
            print(error_message)
            print('project userdata:')
            pprint(project.userdata)
            errors = True
            continue

        upstream_sha = get_upstream_sha(project)
        if not upstream_sha:
            print('OK')
            continue

        if not project.is_ancestor_of(upstream_sha,
                                      'refs/heads/manifest-rev'):
            print(f'error: invalid "upstream-sha: {upstream_sha}"\n'
                  "This must be the ancestor of the project's current revision "
                  f'({project.revision}) which is the upstream merge-base.\n'
                  'To fix, update the upstream-sha value to the correct commit.')
            errors = True
            continue

        print('OK')

    sys.exit(errors)

if __name__ == '__main__':
    main()
