#!/bin/bash
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

set -e

#GITHUB_TOKEN= from keyboard if commented
GITHUB_ACTOR=...user...
GITHUB_REPO=...user_or_organization/repo_name...
PR_NUMBER=...number...
GITHUB_RUN_ID=...number...

rm -Rf /tmp/test-pr-api-check
mkdir -p /tmp/test-pr-api-check
echo '{ "notice": 1, "warning": 3, "critical": 0 }' > /tmp/test-pr-api-check/headers.stats.json
echo '{ "notice": 1, "warning": 0, "critical": 1 }' > /tmp/test-pr-api-check/dts.stats.json

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

if [ -z "$GITHUB_TOKEN" ]; then
    read -p "GITHUB_TOKEN: " -s GITHUB_TOKEN
fi

export GITHUB_TOKEN
export GITHUB_ACTOR
export GITHUB_REPO
export PR_NUMBER
export GITHUB_RUN_ID

python3 $SCRIPT_DIR /tmp/test-pr-api-check/headers.stats.json /tmp/test-pr-api-check/dts.stats.json
