#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
Dynamic Check Discovery System

This module provides functionality to automatically discover and load all check classes
from the checks directory. It dynamically imports all Python files from the checks folder
and identifies classes that inherit from MatterSampleTestCase.
"""

import importlib
import inspect
from pathlib import Path

from internal.check import MatterSampleTestCase


class CheckDiscovery:
    """
    Discovers and loads all check classes from the checks directory.
    """

    def __init__(self, checks_directory: Path):
        """
        Initialize the CheckDiscovery with a checks directory.

        Args:
            checks_directory: Path to the directory containing check modules
        """
        self.checks_directory = checks_directory
        self.check_classes: list[type[MatterSampleTestCase]] = []
        self.doc_check_classes: list[type[MatterSampleTestCase]] = []

    def discover_checks(self) -> list[type[MatterSampleTestCase]]:
        """
        Discover all check classes in the checks directory (excluding subdirectories).

        Returns:
            List of check classes that inherit from MatterSampleTestCase
        """
        self.check_classes = []

        # Find all Python files in the checks directory (not in subdirectories)
        python_files = [
            f for f in self.checks_directory.glob('*.py') if f.is_file() and f.name != '__init__.py'
        ]

        for python_file in python_files:
            module_name = python_file.stem
            try:
                # Dynamically import the module
                module = importlib.import_module(f'checks.{module_name}')

                # Find all classes in the module that inherit from MatterSampleTestCase
                for _, obj in inspect.getmembers(module, inspect.isclass):
                    # Check if it's a subclass of MatterSampleTestCase
                    if (
                        issubclass(obj, MatterSampleTestCase)
                        and obj is not MatterSampleTestCase
                        and obj.__module__ == module.__name__
                    ):
                        self.check_classes.append(obj)

            except Exception as e:
                # Log import errors but continue with other checks
                print(f"Warning: Could not import check from {python_file.name}: {e}")

        return self.check_classes

    def discover_doc_checks(self, subdirectory: str = 'docs') -> list[type[MatterSampleTestCase]]:
        """
        Discover all check classes in a subdirectory of the checks directory.
        These checks are intended to run once per test session, not per sample.

        Args:
            subdirectory: Name of the subdirectory containing doc checks (default: 'docs')

        Returns:
            List of doc check classes that inherit from MatterSampleTestCase
        """
        self.doc_check_classes = []

        docs_directory = self.checks_directory / subdirectory
        if not docs_directory.exists() or not docs_directory.is_dir():
            return self.doc_check_classes

        # Find all Python files in the docs subdirectory
        python_files = [
            f for f in docs_directory.glob('*.py') if f.is_file() and f.name != '__init__.py'
        ]

        for python_file in python_files:
            module_name = python_file.stem
            try:
                # Dynamically import the module from checks.docs
                module = importlib.import_module(f'checks.{subdirectory}.{module_name}')

                # Find all classes in the module that inherit from MatterSampleTestCase
                for _, obj in inspect.getmembers(module, inspect.isclass):
                    # Check if it's a subclass of MatterSampleTestCase
                    if (
                        issubclass(obj, MatterSampleTestCase)
                        and obj is not MatterSampleTestCase
                        and obj.__module__ == module.__name__
                    ):
                        self.doc_check_classes.append(obj)

            except Exception as e:
                # Log import errors but continue with other checks
                print(
                    f"Warning: Could not import doc check from {subdirectory}/{python_file.name}: \
                    {e}"
                )

        return self.doc_check_classes

    def instantiate_check(self, check_class: type[MatterSampleTestCase]) -> MatterSampleTestCase:
        return check_class()
