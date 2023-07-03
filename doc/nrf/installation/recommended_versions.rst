.. _gs_recommended_versions:
.. _requirements:

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

   Before you start setting up the |NCS| toolchain, install available updates for your operating system.

.. _gs_supported_OS:
.. _supported_OS:

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

|NCS| toolchain
***************

The |NCS| :term:`toolchain` includes the Zephyr SDK and then adds on top of it tools and modules required to build |NCS| samples and applications.

Required tools
==============

The following table shows the tools that are required for working with |NCS| v\ |version|.
It lists the versions that are used for testing and installed when using the :ref:`Toolchain Manager <auto_installation>`.
Other versions might also work, but are not verified.

.. _req_tools_table:

.. tabs::

   .. group-tab:: Windows

      .. list-table::
         :header-rows: 1

         * - Tool
           - Toolchain Manager version
         * - Zephyr SDK
           - :ncs-tool-version:`ZEPHYR_SDK_VERSION_WIN10`
         * - CMake
           - :ncs-tool-version:`CMAKE_VERSION_WIN10`
         * - dtc
           - :ncs-tool-version:`DTC_VERSION_WIN10`
         * - Git
           - :ncs-tool-version:`GIT_VERSION_WIN10`
         * - gperf
           - :ncs-tool-version:`GPERF_VERSION_WIN10`
         * - ninja
           - :ncs-tool-version:`NINJA_VERSION_WIN10`
         * - Python
           - :ncs-tool-version:`PYTHON_VERSION_WIN10`
         * - West
           - :ncs-tool-version:`WEST_VERSION_WIN10`

   .. group-tab:: Linux

      .. list-table::
         :header-rows: 1

         * - Tool
           - Toolchain Manager version
         * - Zephyr SDK
           - :ncs-tool-version:`ZEPHYR_SDK_VERSION_LINUX`
         * - CMake
           - :ncs-tool-version:`CMAKE_VERSION_LINUX`
         * - dtc
           - :ncs-tool-version:`DTC_VERSION_LINUX`
         * - Git
           - :ncs-tool-version:`GIT_VERSION_LINUX`
         * - gperf
           - :ncs-tool-version:`GPERF_VERSION_LINUX`
         * - ninja
           - :ncs-tool-version:`NINJA_VERSION_LINUX`
         * - Python
           - :ncs-tool-version:`PYTHON_VERSION_LINUX`
         * - West
           - :ncs-tool-version:`WEST_VERSION_LINUX`

   .. group-tab:: macOS

      .. list-table::
         :header-rows: 1

         * - Tool
           - Toolchain Manager version
         * - Zephyr SDK
           - :ncs-tool-version:`ZEPHYR_SDK_VERSION_DARWIN`
         * - CMake
           - :ncs-tool-version:`CMAKE_VERSION_DARWIN`
         * - dtc
           - :ncs-tool-version:`DTC_VERSION_DARWIN`
         * - Git
           - :ncs-tool-version:`GIT_VERSION_DARWIN`
         * - gperf
           - :ncs-tool-version:`GPERF_VERSION_DARWIN`
         * - ninja
           - :ncs-tool-version:`NINJA_VERSION_DARWIN`
         * - Python
           - :ncs-tool-version:`PYTHON_VERSION_DARWIN`
         * - West
           - :ncs-tool-version:`WEST_VERSION_DARWIN`

Required Python dependencies
============================

The following table shows the Python packages that are required for working with |NCS| v\ |version|.
If no version is specified, the default version provided with pip is recommended.
If a version is specified, it is important that the installed version matches the required version.

The :ref:`Toolchain Manager <auto_installation>` will install all Python dependencies into a local environment in the Toolchain Manager app, not the system.
If you install manually, see :ref:`additional_deps` for instructions on how to install the Python dependencies and :ref:`updating` for information about how to keep them updated.

Building and running applications, samples, and tests
-----------------------------------------------------

.. list-table::
   :header-rows: 1

   * - Package
     - Version
   * - anytree
     - :ncs-tool-version:`ANYTREE_VERSION`
   * - canopen
     - :ncs-tool-version:`CANOPEN_VERSION`
   * - cbor2
     - :ncs-tool-version:`CBOR2_VERSION`
   * - click
     - :ncs-tool-version:`CLICK_VERSION`
   * - cryptography
     - :ncs-tool-version:`CRYPTOGRAPHY_VERSION`
   * - ecdsa
     - :ncs-tool-version:`ECDSA_VERSION`
   * - imagesize
     - :ncs-tool-version:`IMAGESIZE_VERSION`
   * - intelhex
     - :ncs-tool-version:`INTELHEX_VERSION`
   * - packaging
     - :ncs-tool-version:`PACKAGING_VERSION`
   * - progress
     - :ncs-tool-version:`PROGRESS_VERSION`
   * - pyelftools
     - :ncs-tool-version:`PYELFTOOLS_VERSION`
   * - pylint
     - :ncs-tool-version:`PYLINT_VERSION`
   * - PyYAML
     - :ncs-tool-version:`PYYAML_VERSION`
   * - west
     - :ncs-tool-version:`WEST_VERSION`
   * - windows-curses (only Windows)
     - :ncs-tool-version:`WINDOWS_CURSES_VERSION`

.. _python_req_documentation:

Building documentation
----------------------

.. list-table::
   :header-rows: 1

   * - Package
     - Version
   * - recommonmark
     - :ncs-tool-version:`RECOMMONMARK_VERSION`
   * - sphinxcontrib-mscgen
     - :ncs-tool-version:`SPHINXCONTRIB_MSCGEN_VERSION`
   * - breathe
     - :ncs-tool-version:`BREATHE_VERSION`
   * - sphinx
     - :ncs-tool-version:`SPHINX_VERSION`
   * - sphinx-ncs-theme
     - :ncs-tool-version:`SPHINX_NCS_THEME_VERSION`
   * - sphinx-tabs
     - :ncs-tool-version:`SPHINX_TABS_VERSION`
   * - sphinxcontrib-svg2pdfconverter
     - :ncs-tool-version:`SPHINXCONTRIB_SVG2PDFCONVERTER_VERSION`
