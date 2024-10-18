# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import os
import sys
import concurrent.futures
from typing import Callable, Iterable


def warning(*args, **kwargs):
    args = ('\x1B[33mwarning:\x1B[0m', *args)
    print(*args, **kwargs, file=sys.stderr)


def error(*args, **kwargs):
    args = ('\x1B[31merror:\x1B[0m', *args)
    print(*args, **kwargs, file=sys.stderr)


process_executor = None
thread_executor = None


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
    global process_executor, thread_executor, executor_workers
    collected = iterable if isinstance(iterable, tuple) else tuple(iterable)
    if len(collected) >= threshold:
        executor_workers = os.cpu_count() #args.processes if args.processes > 0 else os.cpu_count()
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
        chunksize = (len(collected) + executor_workers - 1) // executor_workers
        it = executor.map(func, collected, chunksize=chunksize)
    else:
        it = map(func, collected)
    return zip(it, collected, range(len(collected)))
