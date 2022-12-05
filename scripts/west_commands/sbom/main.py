#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Main entry point for the script.
'''

from pathlib import Path
import spdx_tag_detector
import full_text_detector
import scancode_toolkit_detector
import cache_database_detector
import external_file_detector
import git_info_detector
import file_input
import input_build
import input_post_process
import output_pre_process
import output_template
from west import log
from common import SbomException, dbg_time
from args import args, init_args
from data_structure import Data


inputs = {
    'input-build': input_build.generate_input,
    'file-input': file_input.generate_input,
}

detectors = {
    'spdx-tag': spdx_tag_detector.detect,
    'git-info': git_info_detector.detect,
    'full-text': full_text_detector.detect,
    'scancode-toolkit': scancode_toolkit_detector.detect,
    'cache-database': cache_database_detector.detect,
    'external-file': external_file_detector.detect,
}

generators = {
    'html': 'templates/report.html.jinja',
    'spdx': 'templates/raport.spdx.jinja',
    'cache_database': 'templates/cache.database.jinja',
}


def main():
    '''Main entry function for the script.'''
    try:
        init_args(detectors)

        data = Data()

        for input_name, input in inputs.items():
            t = dbg_time(f'INPUT: {input_name}')
            input(data)
            log.dbg(f'INPUT: Done in {t}s')

        input_post_process.post_process(data)

        for detector_name in args.license_detectors:
            func = detectors[detector_name]
            optional = detector_name in args.optional_license_detectors
            t = dbg_time(f'DETECTOR: {detector_name}')
            func(data, optional)
            log.dbg(f'DETECTOR: Done in {t}s')

        output_pre_process.pre_process(data)

        for generator_name, generator in generators.items():
            output_file = args.__dict__[f'output_{generator_name}']
            if output_file is not None:
                t = dbg_time(f'GENERATOR: {generator_name}')
                output_template.generate(data, output_file, Path(__file__).parent / generator)
                log.dbg(f'GENERATOR: Done in {t}s')
    except SbomException as e:
        log.die(str(e), exit_code=1)


if __name__ == '__main__':
    main()
