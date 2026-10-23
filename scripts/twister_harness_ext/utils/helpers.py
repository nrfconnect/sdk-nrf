# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Helper functions and decorators for test utilities and board management."""

from __future__ import annotations

import functools
import logging
import os
import time

logger = logging.getLogger(__name__)


def retry(retry_num: int):
    """Retry the function if it raises an exception.

    Retries the decorated function up to `retry_num` times if an exception is raised.
    """

    def decorator_retry(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            attempt = 0
            while True:
                try:
                    return func(*args, **kwargs)
                except Exception:
                    if attempt < retry_num:
                        attempt += 1
                        logger.exception("Exception occurred in %s", func)
                        time.sleep(1)
                        logger.info(f"Trying attempt {attempt}/{retry_num}")
                    else:
                        raise

        return wrapper

    return decorator_retry


def timer(func):
    """Measure execution time of the function."""

    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        start_time = time.time()
        result = func(*args, **kwargs)
        end_time = time.time()
        execution_time = end_time - start_time
        logger.debug(f"Execution time of {func.__name__}: {execution_time:.3f} seconds")
        return result

    return wrapper


def normalize_path(path: str | None) -> str | None:
    """Normalize and expand a filesystem path."""
    if path is not None:
        path = os.path.expanduser(os.path.expandvars(path))
        path = os.path.normpath(os.path.abspath(path))
    return path
