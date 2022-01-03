.. _gs_recommended_versions:

Requirements
############

.. contents::
   :local:
   :depth: 2

The |NCS| supports Microsoft Windows, Linux, and macOS for development.
However, there are some Zephyr features that are currently only available on Linux, including:

* sanitycheck
* BlueZ integration
* net-tools integration

.. note::

   .. _gs_update_os:

   Before you start setting up the toolchain, install available updates for your operating system.

Required tools
**************

The following table shows the tools that are required for working with |NCS| v\ |version|.
It lists the minimum version that is required and the version that is installed when using the :ref:`gs_app_tcm` as described in :ref:`gs_assistant`.

.. _req_tools_table:

.. tabs::

   .. group-tab:: Windows

      .. list-table::
         :header-rows: 1

         * - Tool
           - Minimum version
           - Toolchain Manager version
         * - CMake
           - |cmake_min_ver|
           - |cmake_recommended_ver_win10|
         * - dtc
           - |dtc_min_ver|
           - |dtc_recommended_ver_win10|
         * - git
           -
           - |git_recommended_ver_win10|
         * - GNU Arm Embedded Toolchain
           - |gnuarmemb_min_ver|
           - |gnuarmemb_recommended_ver_win10|
         * - gperf
           - |gperf_min_ver|
           - |gperf_recommended_ver_win10|
         * - ninja
           - |ninja_min_ver|
           - |ninja_recommended_ver_win10|
         * - Python
           - |python_min_ver|
           - |python_recommended_ver_win10|
         * - SEGGER Embedded Studio (Nordic Edition)
           - |ses_min_ver|
           - |ses_recommended_ver_win10|
         * - west
           - |west_min_ver|
           - |west_recommended_ver_win10|
         * - GN
           - |gn_min_ver|
           - |gn_recommended_ver_win10|

   .. group-tab:: Linux

      .. list-table::
         :header-rows: 1

         * - Tool
           - Minimum version
           - Tested version
         * - ccache
           -
           - |ccache_recommended_ver_linux|
         * - CMake
           - |cmake_min_ver|
           - |cmake_recommended_ver_linux|
         * - dfu_util
           -
           - |dfu_util_recommended_ver_linux|
         * - dtc
           - |dtc_min_ver|
           - |dtc_recommended_ver_linux|
         * - git
           -
           - |git_recommended_ver_linux|
         * - GNU Arm Embedded Toolchain
           - |gnuarmemb_min_ver|
           - |gnuarmemb_recommended_ver_linux|
         * - gperf
           - |gperf_min_ver|
           - |gperf_recommended_ver_linux|
         * - ninja
           - |ninja_min_ver|
           - |ninja_recommended_ver_linux|
         * - Python
           - |python_min_ver|
           - |python_recommended_ver_linux|
         * - SEGGER Embedded Studio (Nordic Edition)
           - |ses_min_ver|
           - |ses_recommended_ver_linux|
         * - west
           - |west_min_ver|
           - |west_recommended_ver_linux|
         * - GN
           - |gn_min_ver|
           - |gn_recommended_ver_linux|

   .. group-tab:: macOS

      .. list-table::
         :header-rows: 1

         * - Tool
           - Minimum version
           - Toolchain Manager version
         * - CMake
           - |cmake_min_ver|
           - |cmake_recommended_ver_darwin|
         * - dtc
           - |dtc_min_ver|
           - |dtc_recommended_ver_darwin|
         * - git
           -
           - |git_recommended_ver_darwin|
         * - GNU Arm Embedded Toolchain
           - |gnuarmemb_min_ver|
           - |gnuarmemb_recommended_ver_darwin|
         * - gperf
           - |gperf_min_ver|
           - |gperf_recommended_ver_darwin|
         * - ninja
           - |ninja_min_ver|
           - |ninja_recommended_ver_darwin|
         * - Python
           - |python_min_ver|
           - |python_recommended_ver_darwin|
         * - SEGGER Embedded Studio (Nordic Edition)
           - |ses_min_ver|
           - |ses_recommended_ver_darwin|
         * - west
           - |west_min_ver|
           - |west_recommended_ver_darwin|
         * - GN
           - |gn_min_ver|
           - |gn_recommended_ver_darwin|

Required Python dependencies
****************************

The following table shows the Python packages that are required for working with |NCS| v\ |version|.
If no version is specified, the default version provided with pip is recommended.
If a version is specified, it is important that the installed version matches the required version.

The :ref:`gs_app_tcm` will install all Python dependencies into a local environment in the Toolchain Manager app, not the system.
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
