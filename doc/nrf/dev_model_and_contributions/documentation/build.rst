.. _doc_build:

Building the |NCS| documentation
################################

.. contents::
   :local:
   :depth: 2

The |NCS| documentation is written using the reStructuredText markup language (:file:`.rst` file extension) with Sphinx extensions and processed using Sphinx.
API documentation is generated from Doxygen comments.

See :ref:`zephyr:documentation-overview` in the Zephyr developer guide for information about reStructuredText.

Before you start
****************

Before you can build the documentation, you must install the required tools.
The following tool versions have been tested to work:

* Doxygen :ncs-tool-version:`DOXYGEN_VERSION_LINUX`
* Mscgen 0.20
* PlantUML
* Python dependencies as listed in :ref:`python_req_documentation` on the Requirements page

Complete the following steps to install the required tools:

1. If you have not done so already, install the |NCS| as described in :ref:`install_ncs`.
#. Install or update all required :ref:`Python dependencies <requirements_toolchain_python_deps>`.
#. Install the additional Python dependencies for building the documentation by entering the following command in a terminal window in the :file:`ncs` folder.
   `Python virtual environment <Python virtual environments_>`_ needs to be active for the command to work:

   .. code-block:: bash

      pip install -U -r nrf/doc/requirements.txt

#. Install `Doxygen`_.
#. Install `Mscgen`_ and make sure that the ``mscgen`` executable is in your :envvar:`PATH`.
#. Install PlantUML.
   On Windows, you can install `PlantUML from chocolatey`_.

.. _doc_build_steps:

Building documentation output
*****************************

All documentation build files are located in the :file:`ncs/nrf/doc` folder.
The :file:`nrf` subfolder in that directory contains all :file:`.rst` source files that are not directly related to a sample application or a library.
Documentation for samples and libraries is provided in a :file:`README.rst` or another :file:`.rst` file in the same directory as the code.

Building the documentation output requires building the output for all documentation sets that are part of the :ref:`doc_structure`.
Since there are links from the |NCS| documentation set into other documentation sets, the documentation is built in a predefined order.

Complete the following steps to build the documentation output:

1. |open_terminal_window_with_environment|
#. Enter the doc folder :file:`ncs/nrf/doc`.
#. Generate the Ninja build files by entering the following command:

   .. code-block:: console

      cmake -GNinja -S. -B_build

#. Enter the generated build folder:

   .. code-block:: console

      cd _build

#. Run ninja to build the complete documentation by entering the following command:

   .. code-block:: console

      ninja

The documentation output is written to the :file:`doc/_build/html` folder.
Double-click the :file:`index.html` file to display the documentation in your browser.

Alternatively, you can work with just a single documentation set, for example, ``nrf``.
The build system provides individual targets for such a purpose.
If you have not built all documentation sets before, it is recommended to run the following command:

.. parsed-literal::

   ninja *docset-name*-all

Here, *docset-name* is the name of the documentation set, for example, ``nrf``.
This target will build the :ref:`documentation sets <documentation_sets>` that are needed for *docset-name*.
Note that Doxygen-only docsets like ``nrfx`` do not have the ``-all`` target as they have no dependencies.

On subsequent builds, it is recommended to just run the following command:

.. parsed-literal::

   ninja *docset-name*

The last couple of targets mentioned in :ref:`documentation_sets` will only invoke the build for the corresponding documentation set (referred by *docset-name*), assuming that all of its dependencies are available.

Additionally, the ``*docset-name*-live-all`` and ``*docset-name*-live`` targets are provided with equal functionality plus hot reloading.
The advantage of using ``live`` targets is that by just editing and saving changes, a re-build will be triggered and browser window will be refreshed.

.. _caching_and_cleaning:

Caching and cleaning
********************

To speed up the documentation build, Sphinx processes only those files that have been changed since the last build.
This mechanism can sometimes cause issues such as navigation not being updated correctly.

If you experience any of such issues, clean the build folders before you run the documentation build.

To clean all the build files:

.. code-block:: console

   ninja clean

To clean the build folders for a particular documentation set:

.. parsed-literal::

   ninja *docset-name*-clean

Here, *docset-name* is the name of the documentation set, for example, ``nrf``.


.. _optimizing_doc_build_speed:

Optimizing build speed
**********************

The documentation will by default skip generating certain parts like devicetree bindings or hardware feature tables.
These options are enabled in CI when building the documentation before it is published.

You can try them locally by enabling ``-DDTS_BINDINGS=ON`` and ``-DHW_FEATURES=ON``.

.. _building_kconfig_diff:

Building the Kconfig diff
*************************

The Kconfig Reference documentation set can include a :ref:`kconfig:kconfig_diff` page that lists the changes in Kconfig configuration options between the current tree and the previous |NCS| release.
Generating this diff is disabled by default, because it loads the complete Kconfig tree and downloads the Kconfig snapshot of the previous release over the network.
To enable it, set the ``KCONFIGDIFF`` CMake option to ``ON`` when generating the build files:

.. code-block:: console

   cmake -GNinja -S. -B_build -DKCONFIGDIFF=ON

.. _testing_versions:

Testing locally
***************

Documentation sets for different versions of the |NCS| are defined in the :file:`doc/versions.json` file.
This file is used to display the :ref:`version drop-down <doc_structure_versions>`.

To test the version drop-down locally, complete the following steps:

1. In the documentation build folder (for example, :file:`_build`), rename the :file:`html` folder to :file:`latest`.
#. Open a command-line window in the documentation build folder and enter the following command to start a Python web server::

      python -m http.server

#. Access http://localhost:8000/latest/index.html with your browser to see the documentation.

To add other versions of the documentation to your local documentation output, build the versions from a tagged release and rename the :file:`html` folder to the respective version (for example, |release_number_tt|).

nRF Util command examples
=========================

A pull request check, separate from the documentation build, checks the structure of nRF Util commands in literal blocks, ``code-block`` directives, and ``parsed-literal`` directives.
A line is treated as a command if it starts with ``nrfutil``, optionally preceded by a ``$`` or ``>`` prompt.
The check reports subcommands and options that do not exist in the nRF Util version used by the |NCS|.
It does not check option values, so you can use placeholders such as ``<serial_number>``.

To run the check locally, use the following command from the :file:`nrf` directory:

.. code-block:: console

   python doc/_scripts/validate_nrfutil_commands.py doc/nrf/installation/install_ncs.rst

The check installs nRF Util and its commands in a temporary location, using the versions from the :file:`scripts/tools-versions-<os>.yml` files and the ``nrfutil_<command>_ver`` replacements in :file:`doc/nrf/shortcuts.txt`.
Files in the release notes and migration guides are not checked, because they describe commands from earlier versions.
Commands in Windows, Linux, and macOS tabs are checked only on the matching operating system.

In a pull request, the check runs only on the RST files that the pull request changes.
It checks all files if the pull request changes the check itself, :file:`doc/nrf/shortcuts.txt`, or the :file:`scripts/tools-versions-<os>.yml` files.

A weekly workflow in the nRF Util repository also checks all files on the ``main`` branch, both with the pinned nRF Util versions and with the latest ones.
For each subcommand or option that is not found, it creates a documentation task in Jira that lists the affected pages.

If a command must not be checked, or is only available on some operating systems, add one of the following comments before the block that contains it:

* ``.. validate command: skip next`` - Skips the next command.
* ``.. validate command: windows``, ``linux``, ``macos``, or ``unix`` - Checks the next command only on the given operating system.

The comment applies only to the next command, so add it before each command that needs it.
For example:

.. code-block:: none

   .. validate command: skip next

   .. code-block:: console

      nrfutil device list
