.. _west_sbom:

Software Bill of Materials
##########################

.. contents::
   :local:
   :depth: 2

The Software Bill of Materials (SBOM) is a :ref:`west <zephyr:west>` extension command that can be invoked by ``west ncs-sbom``.
It provides a list of used licenses for an application build or specific files.

.. note::
    Generating a list of licenses from an application build is :ref:`experimental <software_maturity>`.
    The accuracy of detection is constantly verified.
    Both implementation and usage may change in the future.

Overview
********

The process of using the ``ncs-sbom`` command involves the following steps:

#. Create a list of input files based on provided command-line arguments,
   for example, all source files used for building a specific application.
   For details, see :ref:`west_sbom Specifying input`.

#. Detect the license applied to each file,
   for example, read `SPDX identifier`_ from ``SPDX-License-Identifier`` tag.
   For details, see :ref:`west_sbom Detectors`.

#. Create an output report containing all the files and license information related to them,
   for example, write a report file in HTML format.
   For details, see :ref:`west_sbom Specifying output`.

Requirements
************

The ``ncs-sbom`` command requires installation of additional Python packages.

Use the following command to install the requirements.

.. tabs::

   .. group-tab:: Windows

      Enter the following command in a command-line window in the :file:`ncs` folder:

        .. parsed-literal::
           :class: highlight

           pip3 install -r nrf/scripts/requirements-west-ncs-sbom.txt

   .. group-tab:: Linux

      Enter the following command in a terminal window in the :file:`ncs` folder:

        .. parsed-literal::
           :class: highlight

           pip3 install --user -r nrf/scripts/requirements-west-ncs-sbom.txt

   .. group-tab:: macOS

      Enter the following command in a terminal window in the :file:`ncs` folder:

        .. parsed-literal::
           :class: highlight

           pip3 install -r nrf/scripts/requirements-west-ncs-sbom.txt

.. note::
   The ``ncs-sbom`` command uses the `Scancode-Toolkit`_ that requires installation of additional dependencies on a Linux system.
   To install the required tools on Ubuntu, run::

      sudo apt install python-dev bzip2 xz-utils zlib1g libxml2-dev libxslt1-dev libpopt0

   For more details, see `Scancode-Toolkit Installation`_.

Using the command
*****************

The following examples demonstrate the basic usage of the ``ncs-sbom`` command.

* To see the help, run the following command:

  .. code-block:: bash

     west ncs-sbom -h

* To get an analysis of the built application and generate a report to the :file:`sbom_report.html` file in the build directory, run:

  .. parsed-literal::
     :class: highlight

      west ncs-sbom -d *build-directory*

* To analyze the selected files and generate a report to an HTML file, run:

  .. parsed-literal::
     :class: highlight

     west ncs-sbom --input-files *file1* *file2* --output-html *file-name.html*

.. _west_sbom Specifying input:

Specifying input
================

You can specify all input options several times to provide more input for the report generation, for example, generate a report for two applications.
You can also mix them, for example, to generate a report for the application and some directory.

* To get an application SBOM from a build directory, use the following option:

  .. code-block:: bash

     -d build_directory

  You have to first build the ``build_directory`` with the ``west build`` command using Ninja as the underlying build tool (default).
  The build must be successful.
  Any change in the application configuration may affect the results, so always rebuild it after reconfiguration and before calling the ``west ncs-sbom``.

  You can skip this option if you are in the application directory and you have a default :file:`build` directory there - the same way as in ``west build`` command.

  The :ref:`west_sbom Extracting from build` section describes in detail how to extract a list of files from a build directory.

  .. note::
      All files that are not dependencies of the :file:`zephyr/zephyr.elf` target are not taken as an input.
      If you modify the :file:`.elf` file after the linking, the modifications are not applied.

      The ``-d`` option is experimental.

* Provide a list of input files directly on the command line:

  .. parsed-literal::
     :class: highlight

     --input-files *file1* *file2* ...

  Each argument of this option can contain globs as defined by `Python's Path.glob`_ with two additions:

  * Support for absolute paths.
  * Exclamation mark ``!`` to exclude files.

  For example, if you want to include all :file:`.c` files from the current directory and all subdirectories recursively:

  .. code-block:: bash

     --input-files '**/*.c'

  Make sure to have correct quotes around globs, to not have the glob resolved by your shell, and go untouched to the command.

  You can prefix a pattern with the exclamation mark ``!`` to exclude some files.
  Patterns are evaluated from left to right, so ``!`` excludes files from patterns before it, but not after.
  For example, if you want to include all :file:`.c` files from the current directory and all subdirectories recursively except all :file:`main.c` files, run:

  .. code-block:: bash

     --input-files '**/*.c' '!**/main.c'

* Read a list of input files from a file:

  .. parsed-literal::
     :class: highlight

     --input-list-file *list_file*

  It does the same as ``--input-files``, but it reads files and patterns from a file (one file or pattern per line).
  Files and patterns contained in the list file are relative to the list file location (not the current directory).
  Comments starting with a ``#`` character are allowed.


.. _west_sbom Specifying output:

Specifying output
=================

You can specify the format of the report output using the ``output`` argument.

* To generate a report in HTML format:

  .. parsed-literal::
     :class: highlight

     --output-html *file-name.html*

  The :ref:`west_sbom HTML report overview` section provides more details about the report.

  If you use the ``-d`` option, you do not need to specify any output argument.
  The :file:`sbom_report.html` file is generated in your build directory
  (the first one if you specify more than one build directory).

* To generate a cache database:

  .. parsed-literal::
     :class: highlight

     --output-cache-database *cache-database.json*

  For details, see ``cache-database`` detector.

.. _west_sbom Detectors:

Detectors
=========

The ``ncs-sbom`` command includes the following detectors:

* ``spdx-tag`` - Search for the ``SPDX-License-Identifier`` in the source code or the binary file.

  For guidelines, see `SPDX identifier`_. Enabled by default.

* ``full-text`` - Compare the contents of the source file with a small database of reference texts.

  The database is part of the ``ncs-sbom`` command. Enabled by default.

* ``scancode-toolkit`` - License detection by the `Scancode-Toolkit`_. Enabled and optional by default.

  If the ``scancode`` command is not on your ``PATH``, you can use the ``--scancode`` option to provide it, for example:

  .. code-block:: bash

     --scancode ~/scancode-toolkit/scancode

  This detector is optional because it is significantly slower than the others.
  Due to its limitations, it is run on single files one by one using a single process.

* ``external-file`` - Search for license information in an external file. Enabled by default.

  The external file has the following properties:

    * It is located in the same directory as the file under detection or in one of its parent directories .
    * Its name contains ``LICENSE``, ``LICENCE`` or ``COPYING`` (case insensitive).
    * It has an ``SPDX-License-Identifier`` tag.
    * It has one or several ``NCS-SBOM-Apply-To-File`` tags containing file paths or globs (as defined by the `Python's Path.glob`_).
      They are relative to the external file.

  If any of the ``NCS-SBOM-Apply-To-File`` tags matches the file under detection, the license from the SPDX tag is used, for example:

  .. code-block:: text

     /* The following lines will apply Nordic 5-Clause license to all ".a" files
      * and ".lib" files in the "lib" directory and all its subdirectories.
      *
      * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
      * NCS-SBOM-Apply-To-File: lib/**/*.a
      * NCS-SBOM-Apply-To-File: lib/**/*.lib
      */

* ``cache-database`` - Use license information detected and cached earlier in the cache database file.
  Disabled by default.

  Provide the cache database file using the following argument:

  .. parsed-literal::
     :class: highlight

     --input-cache-database *cache-database.json*

  Each database entry has a path relative to the west workspace directory, a hash, and a list of detected licenses.
  If the file under detection has the same path and hash, the list of licenses from the database is used.

  .. note::
     To generate the database based on, for example the ``scancode-toolkit`` detector, run the following command:

     .. parsed-literal::
        :class: highlight

        west ncs-sbom --input-files *files ...* --license-detectors scancode-toolkit --output-cache-database *cache-database.json*

If you prefer a non-default set of detectors, you can provide a list of comma-separated detectors with the ``--license-detectors`` option, for example:

  .. code-block:: bash

     --license-detectors spdx-tag,scancode-toolkit

Some of the detectors are optional, which means that they are not executed for a file that
already has licenses detected by some other previously executed detector.
Detectors are executed from left to right using a list provided by the ``--license-detectors``.

  .. code-block:: bash

     --optional-license-detectors scancode-toolkit

Some detectors may run in parallel on all available CPU cores, which speeds up the detection time.
Use the ``-n`` option to limit the number of parallel threads or processes.

.. _west_sbom HTML report overview:

HTML report overview
********************

The HTML report has following structure:

* Summary of the report, containing the following:

   * Notes at the beginning.

     General information on the report.
   * List of inputs.

     The file sources.
   * List of licenses.

     All licenses detected in the input files.
   * List of added license texts.

     If a license is not in the `SPDX License List`_ and it is in the internal database,
     the license text is added to the report.

  You can click links in the summary to get more details about specific items.

* List of files without any license information or with license information that cannot be detected automatically.

  You have to investigate them manually to get the license information.

* Details about each detected license:

   * License identifier.
   * Information if it is a standard SPDX license.
   * License name if available.
   * Link to license text or more details if available.
   * All files from the input covered by this license.

* License texts added to this report.

.. _west_sbom Extracting from build:

Extracting a list of files from a build directory
*************************************************

The ``ncs-sbom`` extracts a list of files from a build directory.
It queries ninja for the targets and dependencies.

The entry point is the :file:`zephyr/zephyr.elf` target file.
The script asks ninja for all input targets of the :file:`zephyr/zephyr.elf` target.
It also asks for all input targets of the previously extracted input targets,
until it reaches all leaves in the dependency tree.
The result is a list of all the leaves.

To change the target or specify multiple targets, you can add them after the build directory in the ``-d`` option, for example:

.. parsed-literal::
   :class: highlight

   -d build_directory *target1.elf* *target2.elf*

There are two additional methods for improving the correctness of the above algorithm:

* Each library is examined using the GNU ar tool.

  If the list of files returned by the GNU ar tool is covered by the list returned from the ninja, the list is assumed to be valid.
  Otherwise, the library is assumed to be a leaf, so it is shown in the report and its inputs are not analyzed further.

* The ``ncs-sbom`` parses the :file:`.map` file created during the :file:`zephyr/zephyr.elf` linking.

  It provides a list of all object files and libraries linked to the :file:`zephyr/zephyr.elf` file.
  The script ends with a fatal error if any file in the :file:`.map` file is not visible by ninja.

  Exceptions are the runtime and standard libraries.
  You can specify the list of exceptions with the ``--allowed-in-map-file-only`` option.
  By default, it contains a few common names for the runtime and standard libraries.

  If the :file:`.map` file and the associated :file:`.elf` file have different names,
  you can provide the :file:`.map` file after the ``:`` sign following the target,
  for example:

  .. parsed-literal::
     :class: highlight

     -d build_directory *target.elf*:*file.map*
