#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
#
# Script to go through all *.md files in a folder and replace occurrences
# of ".md)" with ".html)" and delete comments.
# Sphinx transforms .md files to .html files, but it does not fix the links
# so they break.
# Sphinx has difficulties parsing multiple comments in a file.

import argparse
import glob
import fileinput
import sys


def main():

    parser = argparse.ArgumentParser(allow_abbrev=False)

    parser.add_argument("dir", nargs=1)
    args = parser.parse_args()

    files = glob.glob(args.dir[0] + "/*.md")

    comment = 0

    for line in fileinput.input(files, inplace=1):
        if "<!--" in line:
            comment = 1

        if "-->" in line:
            comment = 0
            line = ""

        line = line.replace(".md)", ".html)")

        if comment:
            line = ""

        sys.stdout.write(line)


if __name__ == "__main__":
    main()
