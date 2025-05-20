#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Common utility functions used by west ncs-sbom command.
'''

import os
import json
import subprocess
import concurrent.futures
from time import time
from pathlib import Path
from typing import Callable, Iterable
from tempfile import NamedTemporaryFile
from west import log


class SbomException(Exception):
    '''Exception class used by west ncs-sbom to show expected user friendly errors.'''


def command_execute(*cmd_args: 'tuple[str|Path]', cwd: 'str|Path|None' = None,
                    return_path: bool = False, allow_stderr: bool = False,
                    return_error_code: bool = False) -> 'Path|str':
    '''Execute subprocess wrapper that handles errors and output redirections.'''
    cmd_args = tuple(str(x) for x in cmd_args)
    if cwd is not None:
        cwd = str(cwd)
    with NamedTemporaryFile(delete=(not return_path), mode='w+') as out_file, \
            NamedTemporaryFile(mode='w+') as err_file:
        try:
            t = dbg_time(f'Starting {cmd_args} in {cwd or "current directory"}',
                         level=log.VERBOSE_VERY)
            cp = subprocess.run(cmd_args, stdout=out_file, stderr=err_file, cwd=cwd)
            log.dbg(f'Subprocess done in {t}s', level=log.VERBOSE_VERY)
        except Exception as e:
            log.err(f'Running command "{cmd_args[0]}" failed!')
            log.err(f'Arguments: { json.dumps(cmd_args) }, cwd: "{cwd}"')
            log.err(f'Details: {e}')
            raise SbomException('Command execution error.') from e
        out_file.seek(0)
        err_file.seek(0)

        err = err_file.read()

        if len(err.strip()) > 0:
            if allow_stderr:
                log.wrn(f'Command "{cmd_args[0]}" reported errors:\n{err}')
            else:
                log.err(f'Command "{cmd_args[0]}" reported errors:\n{err}')
                if (cp.returncode == 0) or return_error_code:
                    log.err(f'Arguments: { json.dumps(cmd_args) }, cwd: "{cwd}"')
                    raise SbomException('Command execution error.')
        err_file.close()

        if (cp.returncode != 0) and not return_error_code:
            log.err(f'Command "{cmd_args[0]}" exited with error code {cp.returncode}')
            log.err(f'Arguments: { json.dumps(cmd_args) }, cwd: "{cwd}"')
            raise SbomException('Command execution error.')

        if return_path:
            result = Path(out_file.name)
        else:
            result = out_file.read()

        if return_error_code:
            return (result, cp.returncode)
        else:
            return result


process_executor = None
thread_executor = None
executor_workers = None


def concurrent_pool_iter(func: Callable, iterable: Iterable, use_process: bool=False,
                         threshold: int=2):
    ''' Call a function for each item of iterable in a separate thread or process.

    Number of parallel executors will be determined by the CPU count or command line arguments.

    @param func         Function to call
    @param iterable     Input iterator
    @param use_process  Runs function on separate process when True, thread if False
    @param threshold    If number of elements in iterable is less than threshold, no parallel
                        threads or processes will be started.
    @returns            Iterator over tuples cotaining: return value of func, input element, index
                        of that element (starting from 0)
    '''
    from args import args
    global process_executor, thread_executor, executor_workers
    collected = iterable if isinstance(iterable, tuple) else tuple(iterable)
    if len(collected) >= threshold:
        if executor_workers is None:
            executor_workers = args.processes if args.processes > 0 else os.cpu_count()
            if executor_workers is None or executor_workers < 1:
                executor_workers = 1
        if use_process:
            if process_executor is None:
                process_executor = concurrent.futures.ProcessPoolExecutor(executor_workers)
            executor = process_executor
        else:
            if thread_executor is None:
                thread_executor = concurrent.futures.ThreadPoolExecutor(executor_workers)
            executor = thread_executor
        log.dbg(f'Starting {executor_workers} parallel {"processes" if use_process else "threads"}')
        chunksize = (len(collected) + executor_workers - 1) // executor_workers
        it = executor.map(func, collected, chunksize=chunksize)
    else:
        it = map(func, collected)
    return zip(it, collected, range(len(collected)))


def dbg_time(message: 'str|None' = None, level = log.VERBOSE_NORMAL):
    '''Return object that will convert to string containing time elapsed from this function call.
    Optionally it can show a debug log'''
    if log.VERBOSE < level:
        return 0
    class TimeRet:
        def __init__(self, t):
            self.t = t
        def __str__(self) -> str:
            return str(round((time() - self.t) * 100) / 100)
    if message is not None:
        log.dbg(message, level=level)
    return TimeRet(time())
