.. _gs_recommended_versions:

Requirements
############

.. contents::
   :local:
   :depth: 2

The |NCS| supports Microsoft Windows, Linux, and macOS for development.
However, there are some Zephyr features that are currently only available on Linux, including:

* twister
* BlueZ integration
* net-tools integration
* Native Port (native_posix)
* BabbleSim

.. note::

   .. _gs_update_os:

   Before you start setting up the toolchain, install available updates for your operating system.

.. _gs_supported_OS:

Supported operating systems
***************************

The following table shows the operating system versions that support the |NCS| tools:

.. list-table::
   :header-rows: 1

   * - Operating System
     - x86
     - x64
     - ARM64
   * - `Windows 11`_
     - Tier 3
     - Tier 3
     - Not supported
   * - `Windows 10`_
     - Tier 3
     - Tier 1
     - Not supported
   * - `Linux - Ubuntu 22.04 LTS`_
     - Not supported
     - Tier 3
     - Not supported
   * - `Linux - Ubuntu 20.04 LTS`_
     - Not supported
     - Tier 1
     - Not supported
   * - `macOS 13`_
     - Not applicable
     - Tier 3
     - Tier 3
   * - `macOS 12`_
     - Not applicable
     - Tier 2
     - Tier 2
   * - `macOS 11`_
     - Not applicable
     - Tier 1
     - Not supported
   * - `macOS 10.15`_
     - Not applicable
     - Tier 3
     - Not supported

The table uses the following tier definitions to categorize the level of operating system support:

Tier 1
  The |NCS| tools will always work.
  The automated build and automated testing ensure that the |NCS| tools build and successfully complete tests after each change.

Tier 2
  The |NCS| tools will always build.
  The automated build ensures that the |NCS| tools build successfully after each change.
  There is no guarantee that a build will work because the automation tests do not always run.

Tier 3
  The |NCS| tools are supported by design but are not built or tested after each change.
  Therefore, the application may or may not work.

Not supported
  The |NCS| tools do not work, but it may be supported in the future.

Not applicable
  The specified architecture is not supported for the respective operating system.

.. note::
   The |NCS| tools are not supported by the older versions of the operating system.

Required tools
**************

The following table shows the tools that are required for working with |NCS| v\ |version|.
It lists the minimum version that is required and the version that is installed when using the :ref:`Toolchain Manager <gs_app_tcm>`, as described in :ref:`gs_assistant`.

.. _req_tools_table:

.. tabs::

   .. group-tab:: Windows

      .. list-table::
         :header-rows: 1

         * - Tool
           - Minimum version
           - Toolchain Manager version
         * - Zephyr SDK
           - |zephyr_sdk_min_ver|
           - |zephyr_sdk_recommended_ver_win10|
         * - CMake
           - |cmake_min_ver|
           - |cmake_recommended_ver_win10|
         * - dtc
           - |dtc_min_ver|
           - |dtc_recommended_ver_win10|
         * - Git
           - |git_min_ver|
           - |git_recommended_ver_win10|
         * - gperf
           - |gperf_min_ver|
           - |gperf_recommended_ver_win10|
         * - ninja
           - |ninja_min_ver|
           - |ninja_recommended_ver_win10|
         * - Python
           - |python_min_ver|
           - |python_recommended_ver_win10|
         * - West
           - |west_min_ver|
           - |west_recommended_ver_win10|

   .. group-tab:: Linux

      .. list-table::
         :header-rows: 1

         * - Tool
           - Minimum version
           - Toolchain Manager version
         * - Zephyr SDK
           - |zephyr_sdk_min_ver|
           - |zephyr_sdk_recommended_ver_linux|
         * - CMake
           - |cmake_min_ver|
           - |cmake_recommended_ver_linux|
         * - dtc
           - |dtc_min_ver|
           - |dtc_recommended_ver_linux|
         * - Git
           - |git_min_ver|
           - |git_recommended_ver_linux|
         * - gperf
           - |gperf_min_ver|
           - |gperf_recommended_ver_linux|
         * - ninja
           - |ninja_min_ver|
           - |ninja_recommended_ver_linux|
         * - Python
           - |python_min_ver|
           - |python_recommended_ver_linux|
         * - West
           - |west_min_ver|
           - |west_recommended_ver_linux|

   .. group-tab:: macOS

      .. list-table::
         :header-rows: 1

         * - Tool
           - Minimum version
           - Toolchain Manager version
         * - Zephyr SDK
           - |zephyr_sdk_min_ver|
           - |zephyr_sdk_recommended_ver_darwin|
         * - CMake
           - |cmake_min_ver|
           - |cmake_recommended_ver_darwin|
         * - dtc
           - |dtc_min_ver|
           - |dtc_recommended_ver_darwin|
         * - Git
           - |git_min_ver|
           - |git_recommended_ver_darwin|
         * - gperf
           - |gperf_min_ver|
           - |gperf_recommended_ver_darwin|
         * - ninja
           - |ninja_min_ver|
           - |ninja_recommended_ver_darwin|
         * - Python
           - |python_min_ver|
           - |python_recommended_ver_darwin|
         * - West
           - |west_min_ver|
           - |west_recommended_ver_darwin|


Required Python dependencies
****************************

The following table shows the Python packages that are required for working with |NCS| v\ |version|.
If no version is specified, the default version provided with pip is recommended.
If a version is specified, it is important that the installed version matches the required version.

The :ref:`Toolchain Manager <gs_app_tcm>` will install all Python dependencies into a local environment in the Toolchain Manager app, not the system.
If you install manually, see :ref:`additional_deps` for instructions on how to install the Python dependencies and :ref:`gs_updating` for information about how to keep them updated.

Building and running applications, samples, and tests
=====================================================

.. list-table::
   :header-rows: 1

   * - Package
     - Version
   * - anytree
     - |anytree_ver|
   * - canopen
     - |canopen_ver|
   * - cbor2
     - |cbor2_ver|
   * - click
     - |click_ver|
   * - cryptography
     - |cryptography_ver|
   * - ecdsa
     - |ecdsa_ver|
   * - imagesize
     - |imagesize_ver|
   * - intelhex
     - |intelhex_ver|
   * - packaging
     - |packaging_ver|
   * - progress
     - |progress_ver|
   * - pyelftools
     - |pyelftools_ver|
   * - pylint
     - |pylint_ver|
   * - PyYAML
     - |PyYAML_ver|
   * - west
     - |west_ver|
   * - windows-curses (only Windows)
     - |windows-curses_ver|

.. _python_req_documentation:

Building documentation
======================

.. list-table::
   :header-rows: 1

   * - Package
     - Version
   * - recommonmark
     - |recommonmark_ver|
   * - sphinxcontrib-mscgen
     - |sphinxcontrib-mscgen_ver|
   * - breathe
     - |breathe_ver|
   * - sphinx
     - |sphinx_ver|
   * - sphinx-ncs-theme
     - |sphinx-ncs-theme_ver|
   * - sphinx-tabs
     - |sphinx-tabs_ver|
   * - sphinxcontrib-svg2pdfconverter
     - |sphinxcontrib-svg2pdfconverter_ver|
