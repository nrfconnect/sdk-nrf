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

Supported operating systems (firmware)
**************************************

The |NCS| supports Microsoft Windows, Linux, and macOS for development.

The following table lists the support levels for the |NCS| firmware.
For OS support for additional software tools from Nordic Semiconductor, see :ref:`the table at the bottom of the page <additional_nordic_sw_tools>`.

.. os_table_start

.. list-table::
  :header-rows: 1

  * - Operating System
    - x64
    - ARM64
  * - `Windows 11`_
    - Built and tested with :ref:`Twister <running_unit_tests>`.
    - Not supported.
  * - `Linux - Ubuntu 24.04 LTS`_
    - Built and tested with :ref:`Twister <running_unit_tests>`. Comprehensive testing with Nordic Semiconductor hardware.
    - Not supported.
  * - `macOS 14`_
    - Built and tested with :ref:`Twister <running_unit_tests>`.
    - Only toolchain provided.

.. os_table_end

For building, Twister uses definitions in :file:`sample.yml` for the default configuration for the given sample or application.

Zephyr features only available on Linux
  There are some Zephyr features that are currently only available on Linux, including:

  * BlueZ integration
  * net-tools integration
  * Native Port (native_sim)
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

The table lists the versions that are used for testing and are installed with the toolchain bundle when using the :ref:`nRF Connect for VS Code extension or command line <install_ncs>`.
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
         * - nRF Util
           - :ncs-tool-version:`NRFUTIL_VERSION_WIN10`
         * - nRF Util's `device command <Device command overview_>`_
           - :ncs-tool-version:`NRFUTIL_DEVICE_VERSION_WIN10`

   .. group-tab:: Linux

      .. list-table::
         :header-rows: 1

         * - Tool
           - Version
         * - Zephyr SDK
           - :ncs-tool-version:`ZEPHYR_SDK_VERSION_LINUX`
         * - CMake
           - :ncs-tool-version:`CMAKE_VERSION_LINUX`
         * - Devicetree compiler (dtc)
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
         * - nRF Util
           - :ncs-tool-version:`NRFUTIL_VERSION_LINUX`
         * - nRF Util's `device command <Device command overview_>`_
           - :ncs-tool-version:`NRFUTIL_DEVICE_VERSION_LINUX`

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
         * - Devicetree compiler (dtc)
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
         * - nRF Util
           - :ncs-tool-version:`NRFUTIL_VERSION_DARWIN`
         * - nRF Util's `device command <Device command overview_>`_
           - :ncs-tool-version:`NRFUTIL_DEVICE_VERSION_DARWIN`

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
     - :ncs-tool-version:`TQDM_VERSION`
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

.. _requirements_jlink:

J-Link Software and Documentation Pack
**************************************

SEGGER's `J-Link Software and Documentation Pack`_ is a package of tools that is required for SEGGER J-Link to work correctly with both Intel and ARM assemblies.
Among others, this package includes the J-Link RTT Viewer, which can be used for :ref:`test_and_optimize`.

Use the J-Link |jlink_ver| when working with the |NCS|, as also listed in the :ref:`installing_vsc` section on the |NCS| installation page.

On Windows, you also need to install SEGGER USB Driver for J-Link, which is required for support of older Nordic Semiconductor devices in :ref:`requirements_nrf_util`.
For information on how to install the USB Driver, see the `nRF Util prerequisites`_ documentation.

.. _toolchain_management_tools:
.. _additional_nordic_sw_tools:

Additional software tools
*************************

Nordic Semiconductor provides proprietary tools for working with Nordic Semiconductor devices, as well as different |NCS| toolchain management tools that streamline the process of installing the |NCS| and its toolchain.
Depending on your development environment, you need to install only some of them when you :ref:`installing_vsc`.

.. _additional_nordic_sw_tools_os_support:

Supported operating systems (proprietary tools)
===============================================

The following table shows the operating system versions that support the additional software tools from Nordic Semiconductor.
For firmware OS support, see :ref:`the table at the top of the page <supported_OS>`.

.. list-table::
  :header-rows: 1

  * - Operating System
    - x86
    - x64
    - ARM64
  * - `Windows 11`_
    - n/a
    - Tier 1
    - Not supported
  * - `Windows 10`_
    - Tier 3
    - Tier 3
    - Not supported
  * - `Linux - Ubuntu 24.04 LTS`_
    - Not supported
    - Tier 2
    - Not supported
  * - `Linux - Ubuntu 22.04 LTS`_
    - Not supported
    - Tier 1
    - Not supported
  * - `Linux - Ubuntu 20.04 LTS`_
    - Not supported
    - Not supported
    - Not supported
  * - `macOS 26`_
    - n/a
    - Tier 3
    - Tier 3
  * - `macOS 15`_
    - n/a
    - Tier 1
    - Tier 1
  * - `macOS 14`_
    - n/a
    - Tier 3
    - Tier 3
  * - `macOS 13`_
    - n/a
    - Tier 3
    - Tier 3

Tier definitions
  .. toggle:: Support levels

     Tier 1
       The toolchain management tools will always work.
       The automated build and automated testing ensure that the |NCS| tools build and successfully complete tests after each change.

     Tier 2
       The toolchain management tools will always build.
       The automated build ensures that the |NCS| tools build successfully after each change.
       There is no guarantee that a build will work because the automation tests do not always run.

     Tier 3
       The toolchain management tools are supported by design, but are not built or tested after each change.
       Therefore, the application may or may not work.

     Not supported
       The toolchain management tools do not work, but it may be supported in the future.

     Not applicable
       The specified architecture is not supported for the respective operating system.

.. _requirements_nrfvsc:

nRF Connect for Visual Studio Code
==================================

|vsc_extension_description|

In addition, |nRFVSC| provides the following configuration tools for the :ref:`build system components <configuration_system_overview>`:

* For CMake, the `build configuration management <How to work with build configurations_>`_.
* For Devicetree, the `Devicetree Visual Editor <How to work with Devicetree Visual Editor_>`_.
* For Kconfig, the `Kconfig GUI <Configuring with nRF Kconfig_>`_.
* For Zephyr's :ref:`zephyr:optimization_tools`, the interactive `Memory report`_ feature.

The extensions that make up |nRFVSC| are nRF Connect for VS Code, nRF Kconfig, nRF DeviceTree, and nRF Terminal.
While you can install each extension separately, some of them will not work without others and you need all four to use all of the features.

The extensions follow their own `release cycle <latest release notes for nRF Connect for Visual Studio Code_>`_.
Use the latest available release for development.

The extensions are available for download from the following websites:

* |VSC| Marketplace, where the extensions are bundled as the `nRF Connect for VS Code Extension Pack`_.
* `Open VSX Registry`_, from where you can install the extensions separately to editors based on |VSC| and compatible with the VSIX format.

  .. note::
     Nordic Semiconductor does not test editors other than |VSC| for compatibility with |nRFVSC|.
     While you are encouraged to report any issues you encounter on `DevZone`_, issues discovered in editors other than |VSC| and not reproducible in |VSC| will not be prioritized.

See the :ref:`install_ncs` page for information about how to use the extension to manage |NCS| toolchain installations.
For more information about the extension and what it offers, visit the `nRF Connect for Visual Studio Code`_ documentation.

.. _requirements_nrf_util:

nRF Util
========

The `nRF Util development tool`_ is a unified command line utility for Nordic products.
Its functionality is provided through installable and upgradeable commands that are served on a central package registry on the Internet.

The utility follows its own release cycle and has its own `operating system requirements <nRF Util_>`_.

The |NCS| toolchain bundle includes the nRF Util version :ncs-tool-version:`NRFUTIL_VERSION_WIN10` and the device command version :ncs-tool-version:`NRFUTIL_DEVICE_VERSION_WIN10`, as listed in :ref:`requirements_toolchain_tools`.
When you :ref:`gs_installing_toolchain`, you get both these versions `locked <Locking nRF Util home directory_>`_ to prevent unwanted changes to the toolchain bundle.

.. note::

   When you :ref:`install the nRF Connect SDK <install_ncs>`:

   * If you plan to work with command line, you also need to download nRF Util and install the following command in order to get the toolchain bundle:

     * `sdk-manager command`_ - The latest version is required for working with |NCS| toolchain packages.
       See `Installing and upgrading nRF Util commands`_ for information about how to install this command.

   * If you plan to work with the :ref:`nRF Connect for VS Code extension <requirements_nrfvsc>`, you do not need a separate nRF Util installation to get the toolchain bundle.
