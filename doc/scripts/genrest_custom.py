#!/usr/bin/env python3
#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

#
# This tool works as a frontend to Zephyr's genrest.py script. It receives
# a `--custom-modules` argument which is used to populate a list of
# path -> abbreviation resolvers, similarly to the `--modules` parameter of
# Zephyr's genrest.py. It monkey patches Zephyr's genrest `strip_module_path`
# routine to resolve those extra symbols.
#

import argparse
import importlib.util
import os
import os.path
import pathlib
import sys


def parse_custom_modules(args):
    global custom_modules

    custom_modules = []
    for module_spec in args.custom_modules:
        title, path_s = module_spec.split(',')

        abspath = pathlib.Path(path_s).resolve()
        if not abspath.exists():
            sys.exit("error: path '{}' in --custom-modules argument does not "
                     "exist".format(abspath))

        custom_modules.append((title, abspath))


def strip_custom_module_path(path):
    if strip_module_paths:
        abspath = pathlib.Path(kconf.srctree).joinpath(path).resolve()
        for title, mod_path in custom_modules:
            try:
                relpath = abspath.relative_to(mod_path)
            except ValueError:
                continue

            return f"<{title}>{os.path.sep}{relpath}"

    # fallback on Zephyr's resolver
    return strip_module_path(path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--custom-modules', nargs="+", default=[])
    parser.add_argument('--zephyr-genrest')
    args, unknown_args = parser.parse_known_args()

    parse_custom_modules(args)

    # load Zephyr's genrest
    spec = importlib.util.spec_from_file_location("module.name",
                                                  args.zephyr_genrest)
    genrest = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(genrest)

    # call init() from genrest, passing unknown arguments
    sys.argv = [args.zephyr_genrest] + unknown_args
    genrest.init()

    # reuse some of Zephyr's genrest variables and functions
    global kconf
    global strip_module_paths
    global strip_module_path

    kconf = genrest.kconf
    strip_module_paths = genrest.strip_module_paths
    strip_module_path = genrest.strip_module_path

    # monkey patch Zephyr's strip_module_path routine
    genrest.strip_module_path = strip_custom_module_path

    genrest.write_index_pages()

    if os.getenv("KCONFIG_TURBO_MODE") == "1":
        genrest.write_dummy_syms_page()
    else:
        genrest.write_sym_pages()


if __name__ == "__main__":
    main()
