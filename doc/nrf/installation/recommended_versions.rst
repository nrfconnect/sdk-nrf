.. _gs_recommended_versions:
.. _requirements:

Requirements reference
######################

.. contents::
   :local:
   :depth: 2

This page summarizes the requirements for installing and working with the |NCS|.
All of these requirements are installed when you :ref:`install the nRF Connect SDK <install_ncs>`.

.. _gs_supported_OS:
.. _supported_OS:

Supported operating systems
***************************

The |NCS| supports Microsoft Windows, Linux, and macOS for development.
The following table shows the operating system versions that support the |NCS| tools:

.. os_table_start

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
     - Tier 1
     - Not supported
   * - `Linux - Ubuntu 20.04 LTS`_
     - Not supported
     - Tier 2
     - Not supported
   * - `macOS 14`_
     - Not applicable
     - Tier 3
     - Tier 3
   * - `macOS 13`_
     - Not applicable
     - Tier 1
     - Tier 1
   * - `macOS 12`_
     - Not applicable
     - Tier 3
     - Tier 3
   * - `macOS 11`_
     - Not applicable
     - Tier 2
     - Tier 2
   * - `macOS 10.15`_
     - Not applicable
     - Tier 3
     - Not supported

.. os_table_end

Tier definitions
  The table uses several tier definitions to categorize the level of operating system support:

  .. toggle:: Support levels

     Tier 1
       The |NCS| tools will always work.
       The automated build and automated testing ensure that the |NCS| tools build and successfully complete tests after each change.

     Tier 2
       The |NCS| tools will always build.
       The automated build ensures that the |NCS| tools build successfully after each change.
       There is no guarantee that a build will work because the automation tests do not always run.

     Tier 3
       The |NCS| tools are supported by design, but are not built or tested after each change.
       Therefore, the application may or may not work.

     Not supported
       The |NCS| tools do not work, but it may be supported in the future.

     Not applicable
       The specified architecture is not supported for the respective operating system.

Zephyr features only available on Linux
  There are some Zephyr features that are currently only available on Linux, including:

  * BlueZ integration
  * net-tools integration
  * Native Port (native_posix)
  * BabbleSim

.. _requirements_toolchain:

|NCS| toolchain
***************

The |NCS| :term:`toolchain` includes the Zephyr SDK and adds the necessary tools and modules to create |NCS| samples and applications on top of it.
The |NCS| toolchain is installed as one of the steps when :ref:`install_ncs`.

.. note::

   Before you start setting up the |NCS| toolchain, install available updates for your operating system.
   The |NCS| tools are not supported by the older versions of the operating system.

.. _requirements_toolchain_tools:

Required tools
==============

The following table shows the tools that are required for working with |NCS| v\ |version|.

The table lists the versions that are used for testing and are installed when using the :ref:`nRF Connect for VS Code extension or nRF Util <install_ncs>`.
Other versions might also work, but are not verified.

.. _req_tools_table:

.. tabs::

   .. group-tab:: Windows

      .. list-table::
         :header-rows: 1

         * - Tool
           - Version
         * - Zephyr SDK
           - :ncs-tool-version:`ZEPHYR_SDK_VERSION_WIN10`
         * - CMake
           - :ncs-tool-version:`CMAKE_VERSION_WIN10`
         * - Devicetree compiler (dtc)
           - :ncs-tool-version:`DTC_VERSION_WIN10`
         * - :ref:`Git <ncs_git_intro>`
           - :ncs-tool-version:`GIT_VERSION_WIN10`
         * - gperf
           - :ncs-tool-version:`GPERF_VERSION_WIN10`
         * - ninja
           - :ncs-tool-version:`NINJA_VERSION_WIN10`
         * - Python
           - :ncs-tool-version:`PYTHON_VERSION_WIN10`
         * - :ref:`west <ncs_west_intro>`
           - :ncs-tool-version:`WEST_VERSION_WIN10`

   .. group-tab:: Linux

      .. list-table::
         :header-rows: 1

         * - Tool
           - Version
         * - Zephyr SDK
           - :ncs-tool-version:`ZEPHYR_SDK_VERSION_LINUX`
         * - CMake
           - :ncs-tool-version:`CMAKE_VERSION_LINUX`
         * - dtc
           - :ncs-tool-version:`DTC_VERSION_LINUX`
         * - :ref:`Git <ncs_git_intro>`
           - :ncs-tool-version:`GIT_VERSION_LINUX`
         * - gperf
           - :ncs-tool-version:`GPERF_VERSION_LINUX`
         * - ninja
           - :ncs-tool-version:`NINJA_VERSION_LINUX`
         * - Python
           - :ncs-tool-version:`PYTHON_VERSION_LINUX`
         * - :ref:`west <ncs_west_intro>`
           - :ncs-tool-version:`WEST_VERSION_LINUX`

      Additionally, you need to install `nrf-udev`_ rules for accessing USB ports on Nordic Semiconductor devices and programming the firmware.

   .. group-tab:: macOS

      .. list-table::
         :header-rows: 1

         * - Tool
           - Version
         * - Zephyr SDK
           - :ncs-tool-version:`ZEPHYR_SDK_VERSION_DARWIN`
         * - CMake
           - :ncs-tool-version:`CMAKE_VERSION_DARWIN`
         * - dtc
           - :ncs-tool-version:`DTC_VERSION_DARWIN`
         * - :ref:`Git <ncs_git_intro>`
           - :ncs-tool-version:`GIT_VERSION_DARWIN`
         * - gperf
           - :ncs-tool-version:`GPERF_VERSION_DARWIN`
         * - ninja
           - :ncs-tool-version:`NINJA_VERSION_DARWIN`
         * - Python
           - :ncs-tool-version:`PYTHON_VERSION_DARWIN`
         * - :ref:`west <ncs_west_intro>`
           - :ncs-tool-version:`WEST_VERSION_DARWIN`

Checking tool versions
  .. toggle::

     To check the list of installed packages and their versions, run the following command:

     .. tabs::

        .. group-tab:: Windows

           .. code-block:: console

              choco list -lo

           Chocolatey is installed as part of the Zephyr SDK toolchain when you :ref:`install the nRF Connect SDK <install_ncs>`.

        .. group-tab:: Linux

           .. code-block:: console

               apt list --installed

           This command lists all packages installed on your system.
           To list the version of a specific package, type its name and add ``--version``.

        .. group-tab:: macOS

           .. code-block:: console

              brew list --versions

.. _requirements_toolchain_python_deps:

Required Python dependencies
============================

The following table shows the Python packages that are required for working with |NCS| v\ |version|.
If no version is specified, the default version provided with ``pip`` is recommended.
If a version is specified, it is important that the installed version matches the required version.

When you :ref:`install the nRF Connect SDK <install_ncs>`, you will install all Python dependencies into a local environment, not the system.

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

Python documentation dependencies are listed in the following table.
They can all be installed using the ``doc/requirements.txt`` file using ``pip``.

.. list-table::
   :header-rows: 1

   * - Package
     - Version
   * - azure-storage-blob
     - :ncs-tool-version:`AZURE_STORAGE_BLOB_VERSION`
   * - breathe
     - :ncs-tool-version:`BREATHE_VERSION`
   * - m2r2
     - :ncs-tool-version:`M2R2_VERSION`
   * - PyYAML
     - :ncs-tool-version:`PYYAML_VERSION`
   * - pykwalify
     - :ncs-tool-version:`PYKWALIFY_VERSION`
   * - recommonmark
     - :ncs-tool-version:`RECOMMONMARK_VERSION`
   * - sphinx
     - :ncs-tool-version:`SPHINX_VERSION`
   * - sphinx-copybutton
     - :ncs-tool-version:`SPHINX_COPYBUTTON_VERSION`
   * - sphinx-ncs-theme
     - :ncs-tool-version:`SPHINX_NCS_THEME_VERSION`
   * - sphinx-notfound-page
     - :ncs-tool-version:`SPHINX_NOTFOUND_PAGE_VERSION`
   * - sphinx-tabs
     - :ncs-tool-version:`SPHINX_TABS_VERSION`
   * - sphinx-togglebutton
     - :ncs-tool-version:`SPHINX_TOGGLEBUTTON_VERSION`
   * - sphinx_markdown_tables
     - :ncs-tool-version:`SPHINX_MARKDOWN_TABLES_VERSION`
   * - sphinxcontrib-mscgen
     - :ncs-tool-version:`SPHINXCONTRIB_MSCGEN_VERSION`
   * - sphinxcontrib-plantuml
     - :ncs-tool-version:`SPHINXCONTRIB_PLANTUML_VERSION`
   * - west
     - :ncs-tool-version:`WEST_VERSION`

.. _requirements_clt:

nRF Command Line Tools
**********************

`nRF Command Line Tools`_ is a package of tools used for development, programming, and debugging of Nordic Semiconductor's nRF51, nRF52, nRF53, nRF54H, and nRF91 Series devices.
Among others, this package includes the nrfjprog executable and library, which the west command uses by default to program the development kits.
For more information on nrfjprog, see `Programming SoCs with nrfjprog`_.

It is recommended to use the latest version of the package when you :ref:`installing_vsc`.

.. _requirements_jlink:

J-Link Software and Documentation Pack
**************************************

SEGGER's `J-Link Software and Documentation Pack`_ is a package of tools that is required for SEGGER J-Link to work correctly with both Intel and ARM assemblies.
Among others, this package includes the J-Link RTT Viewer, which can be used for :ref:`test_and_optimize`.

It is recommended to use the |jlink_ver| of the package when you :ref:`installing_vsc`.

.. _toolchain_management_tools:

|NCS| toolchain management tools
********************************

Nordic Semiconductor provides proprietary |NCS| toolchain management tools that streamline the process of installing the |NCS| and its toolchain.
Depending on your development environment, you need to install only some of them when you :ref:`installing_vsc`.

|nRFVSC|
========

|vsc_extension_description|

In addition, the |nRFVSC| provides the following configuration tools for the :ref:`build system components <configuration_system_overview>`:

* For CMake, the `build configuration management <How to work with build configurations_>`_.
* For Devicetree, the `Devicetree Visual Editor <How to work with Devicetree Visual Editor_>`_.
* For Kconfig, the `Kconfig GUI <Configuring with nRF Kconfig_>`_.

The extension follows its own `release cycle <latest release notes for nRF Connect for Visual Studio Code_>`_.
Use the latest available release for development.

See the :ref:`install_ncs` page for information about how to use the extension to manage |NCS| toolchain installations.
For more information about the extension and what it offers, visit the `nRF Connect for Visual Studio Code`_ documentation.

.. _requirements_nrf_util:

nRF Util
========

The `nRF Util development tool`_ is a unified command line utility for Nordic products.
Its functionality is provided through installable and upgradeable commands that are served on a central package registry on the Internet.

The utility follows its own release cycle.
Use the latest available release for development.

nRF Util provides |NCS| toolchain packages for each |NCS| release through the ``toolchain-manager`` command.
See the :ref:`install_ncs` page for information about how to use this command.
