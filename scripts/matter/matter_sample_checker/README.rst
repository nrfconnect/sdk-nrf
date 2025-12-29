.. _matter_sample_checker:

Matter Sample Checker
#####################

.. contents::
   :local:
   :depth: 2

The Matter Sample Checker script is a comprehensive consistency checker for Matter samples in the |NCS|.
This tool automatically validates the file structure, configuration consistency, license years, ZAP files, and detects common copy-paste mistakes in Matter samples.

Overview
********

The Matter Sample Checker is designed to enforce consistency and quality across all Matter samples in the nRF Connect SDK.
As the number of Matter samples grows, maintaining uniform structure, configuration, and documentation becomes increasingly challenging.
This tool automates the validation process, catching common issues such as:

- Missing or misconfigured files that break the build or documentation
- Copy-paste errors where references to other samples were not updated
- Outdated license headers or ZAP-generated files
- Deviations from the established Matter sample template

By integrating the checker into a development workflow or CI/CD pipeline, these scipts ensure that new samples and modifications adhere to project standards before they are merged.

Matter Sample Checker features
==============================

The Matter Sample Checker implements the following features:

.. list-table::
   :widths: 30 70
   :header-rows: 1

   * - Feature
     - Description

   * - Dynamic Check Discovery
     - - Automatically discovers and runs all checks from the :file:`checks` directory.
       - No manual registration or imports needed when adding new checks.
       - Create a new check file inheriting from ``MatterSampleTestCase`` to have it included.
       - Smart parameter detection and instantiation for different check types.

   * - File Structure Validation
     - - Verifies presence of required files, for example :file:`CMakeLists.txt`, :file:`prj.conf`, and :file:`sample.yaml`.
       - Checks for required directories (:file:`src`, :file:`sysbuild`, :file:`sysbuild/mcuboot`).
       - Validates optional files and directories.

   * - ZAP Files Validation
     - - Ensures ZAP configuration files (:file:`.zap`, :file:`.matter`) are present.
       - Verifies ZAP-generated files exist and are up-to-date.
       - Checks for proper ZAP directory structure.

   * - License Year Checking
     - - Validates copyright years in source files
       - Supports multiple valid years configuration
       - Checks C/C++/H files and configuration files
       - Skipped by default; enabled with ``--year`` flag

   * - Configuration Consistency
     - - Validates required Matter configurations in the ``prj.conf`` file.
       - Checks the format of the :file:`sample.yaml` and the required tags.
       - Checks the :file:`sample.yaml` format and the required tags.
       - Ensures proper sysbuild configuration
       - Verifies Matter-specific settings

   * - Copy-Paste Error Detection
     - - Dynamically discovers all Matter samples
       - Detects references to other sample names in the current sample
       - Intelligent filtering to avoid false positives
       - Checks documentation, configuration, and source files
       - Supports allowlists for legitimate cross-sample references

   * - Template Comparison
     - - Compares configuration files with the Matter template sample
       - Highlights deviations from the standard template
       - Excludes copyright differences from comparison
       - Provides detailed diff information in verbose mode

Installation
============

The script does not require installation.
It is a standalone Python script that requires Python v3.6+ and the :file:`pyyaml` package for YAML configuration parsing.

- :file:`pyyaml` (for YAML configuration parsing)

.. code-block:: bash

   pip install pyyaml

Usage
*****

Run the script from the command line, pointing it at the Matter sample directory you want to validate.
By default, the script checks the current working directory.
The tool outputs a detailed report of issues, warnings, and informational messages, and returns an exit code indicating the number of issues found.

Run the following command to check the current directory:

.. code-block:: console

    python matter_sample_checker.py

To run the command with more options, run the command with the ``--help`` flag and learn about the `Command-line options`_.

Command-line options
====================

Use the following command-line options to run the script with addtional features:

* ``sample_path`` - Path to Matter sample directory (default: current directory)
* ``--verbose, -v`` - Show verbose output during checks
* ``--year, -y`` - Expected copyright year(s) (enables year checking; omit to skip year checking)
* ``--config, -c`` - Path to custom configuration file
* ``--output, -o`` - Output report to file instead of stdout
* ``--allow-names, -a`` - List of names/terms to allow during copy-paste error checking (case-insensitive)

Configuration
=============

The tool uses a YAML configuration file :file:`matter_sample_checker_config.yaml` that defines the following:

- File structure - Required and optional files and directories
- ZAP configuration - Expected ZAP files and generated content
- License settings - Source file extensions and copyright patterns
- Copy-paste detection - Files to check and patterns to skip
- Validation rules - Required configurations and consistency checks
- Exclusions - Directories and files to exclude from checks

Output format
=============

The tool generates a comprehensive report with the following three types of findings:

Issues (Return code > 0)
   Critical problems that must be fixed

   * Missing required files
   * Configuration errors
   * Copy-paste mistakes

Warnings
   Potential issues that should be reviewed

   * Missing optional files
   * Template deviations
   * Suspicious patterns

Information
   Successful validations and detailed check results

   * Found required files
   * Successful configuration checks
   * Template comparison results

Integration with CI/CD
======================

The tool returns appropriate exit codes for CI/CD integration:

* ``0``: No issues found
* ``>0``: Number of issues found (fails CI/CD pipeline)

Example CI usage:

.. code-block:: console

   python matter_sample_checker.py /path/to/sample || exit 1


Copy-paste error checking
=========================

The copy-paste error detection feature automatically scans for references to other Matter sample names within the current sample.
This helps catch common mistakes where developers copy content from one sample to another and forget to update sample-specific references.

Copy-paste detection features
=============================

By default, the tool performs the following operations:

* Discovers all Matter samples in the SDK.
* Creates a list of "forbidden words" from other sample names.
* Scans files for these references and reports them as potential copy-paste errors.

Using the ``--allow-names`` option
==================================

Use the ``--allow-names`` option when you have legitimate cross-sample references that should not be flagged as errors:

* Documentation references - When your README mentions other samples for comparison.
* Technical discussions - When configuration comments reference other sample patterns.
* Legitimate cross-references - When samples intentionally reference related samples.

Dynamic discovery
=================

The dynamic discovery system allows running all checks from the :file:`checks/` directory automatically.
It scans the :file:`checks/` directory for all :file:`.py` files, dynamically imports each module, and finds classes inheriting from ``MatterSampleTestCase``.
Each check is instantiated with appropriate parameters and run automatically.
No manual registration or imports needed in the main script.

Examples of legitimate use cases
================================

.. code-block:: console

   # Light switch sample legitimately references "light bulb" in documentation
   python matter_sample_checker.py --allow-names "light bulb" /path/to/light_switch

   # Template sample references multiple other samples in documentation
   python matter_sample_checker.py -a template "door lock" "window covering" /path/to/sample

   # Case-insensitive matching works for variations
   python matter_sample_checker.py -a "Light_Bulb" "light bulb" /path/to/sample

Adding new checks
=================

The checker uses the `Dynamic discovery`_ mechanism.
To add a new check, complete the following steps:

1. Create a new check file in the :file:`\checks` directory (for example, :file:`check_my_feature.py`).
#. Inherit from the ``MatterSampleTestCase`` base class.
#. Implement the required methods:

   * ``name()`` - Return the display name of your check.
   * ``prepare()`` - Set up any data or variables needed for the check.
   * ``check()`` - Implement the actual check logic.

#. The checker automatically discovers and runs your new check.

Example check implementation
============================

See the following code snippet as a minimal example of a new check implementation:

.. code-block:: python

    from internal.checker import MatterSampleTestCase
    from internal.utils.defines import MatterSampleCheckerResult
    from typing import List

    class MyFeatureTestCase(MatterSampleTestCase):
        def name(self) -> str:
            return "My Feature Check"

        def prepare(self):
            # Initialize variables needed for the check
            self.files_to_check = []

        def check(self) -> List[MatterSampleCheckerResult]:
            # Implement your check logic here
            if some_condition:
                self.issue("Found an issue")
            else:
                self.debug("Check passed")

Registering results in your check
=================================

Every check inherits helper methods from ``MatterSampleTestCase`` to register findings:

* info(message: str) - General information and successful validation notes.
* debug(message: str) - Detailed progress messages (shown only with ``--verbose``).
* warning(message: str) - Non-fatal problems that should be reviewed.
* issue(message: str) - Definite errors; these increase the exit code and fail CI.

These methods append a ``MatterSampleCheckerResult`` with a level from ``LEVELS`` to the check’s internal result list.
The framework aggregates them and builds the final report:

* Information and debug - Shown under the INFORMATION section (debug only if ``--verbose``).
* Warnings - Counted and listed under WARNINGS.
* Issues - Counted and listed under ISSUES and affect the final result or exit code.

Minimal usage example in ``check()`` might look like this:

.. code-block:: python

    def check(self) -> List[MatterSampleCheckerResult]:
        self.debug("Starting configuration validation")

        if not self._has_required_file:
            self.issue("Missing required file: prj.conf")
            return

        if self._has_suspicious_pattern:
            self.warning("Suspicious pattern detected in README.rst")

        self.info("All required files are present")


Notes and best practices:

* Prefer clear, actionable messages (include which file, configuration, or line is affected).
* Use ``debug()`` for verbose tracing (counts, parsed items, paths) to aid debugging.
* Throwing an exception inside ``check()`` is caught by the framework and recorded as an ``issue()`` with the exception message.
* You do not need to print anything yourself; just use the helper methods.

Custom constructor parameters
=============================

If your check needs additional parameters, you can define a custom ``__init__`` as follows:

.. code-block:: python

    def __init__(self, config: dict, sample_path: Path, skip: bool = False, custom_param: str = None):
        super().__init__(config, sample_path, skip)
        self.custom_param = custom_param

The `Dynamic discovery`_ system automatically detects the parameters and passes them, if available, in the global ``check_parameters`` dictionary.

Modifying configuration
=======================

Update the :file:`matter_sample_checker_config.yaml` file if your new check requires configuration settings.
