#!/usr/bin/env python3
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import os
import sys

def remove_comments_from_assembly_file(input_file):
    """Cleans an assembly file by removing lines starting with # and saves the changes in the same file."""
    with open(input_file, 'r') as infile:
        lines = infile.readlines()

    # Filter out lines starting with #
    cleaned_lines = [line for line in lines if not line.startswith('#')]

    # Save the cleaned file in the same file
    with open(input_file, 'w') as outfile:
        outfile.writelines(cleaned_lines)

    print(f"File {input_file} has been cleaned of comments.")

def process_directory(directory):
    """Processes all .s files in the directory."""
    for root, _, files in os.walk(directory):  # '_' instead of 'dirs' to ignore unused variable
        for file in files:
            if file.endswith('.s'):
                input_file = os.path.join(root, file)
                remove_comments_from_assembly_file(input_file)

def main(path):
    """Checks if given path is a file or a directory and processes it accordingly."""
    if os.path.isfile(path) and path.endswith('.s'):
        # If a single .s file is provided
        remove_comments_from_assembly_file(path)
    elif os.path.isdir(path):
        # If a directory is provided
        process_directory(path)
    else:
        print("The provided path is neither a .s file nor a directory.")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python clean_asm.py <file.or.directory>")
    else:
        main(sys.argv[1])
