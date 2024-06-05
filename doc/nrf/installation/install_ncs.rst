.. _install_ncs:
.. _auto_installation:
.. _gs_installing:

Installing the |NCS|
####################

.. contents::
   :local:
   :depth: 2

There are different ways to install the |NCS|, depending on your preferred development environment and the :ref:`toolchain management tool <toolchain_management_tools>`:

* Using |VSC| and the |nRFVSC| (recommended)
* Using command line and nRF Util

Regardless of which way you choose, the following steps install the |NCS| source code and the |NCS| :term:`toolchain`.
This includes everything that is required by Zephyr's :ref:`zephyr:getting_started` together with additional tools and Python dependencies that the |NCS| uses.

.. note::
    Using |VSC| and the |nRFVSC| is also covered in the `Installing nRF Connect SDK and VS Code`_ exercise of the `nRF Connect SDK Fundamentals course`_ on Nordic Developer Academy.

.. rst-class:: numbered-step

Update operating system
***********************

Before you start setting up the toolchain, install available updates for your operating system.

.. include:: ./recommended_versions.rst
    :start-after: os_table_start
    :end-before: os_table_end

See :ref:`supported_OS` for more information about the tier definitions.

.. _installing_vsc:

.. rst-class:: numbered-step

Install prerequisites
*********************

Depending on your preferred development environment, install the following required tools:

.. tabs::

   .. group-tab:: nRF Connect for Visual Studio Code

      * The latest version of the :ref:`requirements_clt` package.
        Download it from the `nRF Command Line Tools`_ page.
      * The latest version of |VSC| for your operating system from the `Visual Studio Code download page`_.
      * In |VSC|, the latest version of the `nRF Connect for VS Code Extension Pack`_.
      * Linux users: `nrf-udev`_ module with udev rules required to access USB ports on Nordic Semiconductor devices and program the firmware.

   .. group-tab:: Command line

      * The latest version of `nRF Util development tool`_, a unified command line utility for Nordic products.

        .. note::
            After downloading the nRF Util executable, move it to a directory that is in the system :envvar:`PATH`.
            On macOS and Linux, the downloaded file also needs to be given execute permission by typing `chmod +x nrfutil` or by checking the checkbox in the file properties.

      * The latest version of the :ref:`requirements_clt` package.
        Download it from the `nRF Command Line Tools`_ page.

        .. note::
            After downloading and installing the tools, add nrfjprog to the system :envvar:`PATH` in the environment variables.

      * Linux users: `nrf-udev`_ module with udev rules required to access USB ports on Nordic Semiconductor devices and program the firmware.

.. _gs_installing_toolchain:
.. _gs_installing_tools:

.. rst-class:: numbered-step

Install the |NCS| toolchain
***************************

The |NCS| :term:`toolchain` includes the Zephyr SDK and then adds tools and modules required to build |NCS| samples and applications on top of it.
These include the :ref:`required tools <requirements_toolchain_tools>`, the :ref:`Python dependencies <requirements_toolchain_python_deps>`, and the :ref:`GN tool <ug_matter_gs_tools_gn>` for creating :ref:`ug_matter` applications.

When you first install the |NCS|, it is recommended to install the latest released versions of the SDK and the toolchain.

Depending on your preferred development environment, complete the following steps:

.. tabs::

   .. group-tab:: nRF Connect for Visual Studio Code

      1. Open the nRF Connect extension in |VSC| by clicking its icon in the :guilabel:`Activity Bar`.
      #. In the extension's :guilabel:`Welcome View`, click on :guilabel:`Install Toolchain`.
      #. Select the toolchain version to install.
         The toolchain version should match the |NCS| version you are going to work with.
         If you have received a custom URL for installing the toolchain, you can provide it using the :guilabel:`Change Toolchain Index` button in the quick pick's header.
         The toolchain installation starts in the background, as can be seen in the notification that appears.

      When you install the toolchain for the first time, the installed version is automatically selected for your project.

      After installing the toolchain, you can access the :guilabel:`Install Toolchain` option by clicking on :guilabel:`Manage toolchains`.

   .. group-tab:: Command line

      1. Open a terminal window.
      #. Run the following command to install the nRF Util ``toolchain-manager`` command:

         .. code-block:: console

            nrfutil install toolchain-manager

      #. Run the following command to list the available installations:

         .. code-block:: console

            nrfutil toolchain-manager search

         The versions from this list correspond to the |NCS| versions and will be *version* in the following step.
      #. Run the following command to install the toolchain version for the SDK version of your choice:

         .. parsed-literal::
            :class: highlight

            nrfutil toolchain-manager install --ncs-version *version*

         For example:

         .. code-block:: console

            nrfutil toolchain-manager install --ncs-version v2.0.0

         This example command installs the toolchain required for the |NCS| v2.0.0.

      The toolchain is installed by default at :file:`C:/ncs/toolchains` on Windows, :file:`~/ncs/toolchains` on Linux, and :file:`/opt/nordic/ncs/toolchains` on macOS.

      If you have received a custom URL for installing the toolchain, you can use the following command to set it as default, replacing the respective parameter:

      .. parsed-literal::
         :class: highlight

         nrfutil toolchain-manager config --set toolchain-index=\ *custom_toolchain_URL*

      If you have received a custom bundle ID for installing a specific toolchain version, you can use the following commands to provide it, replacing the respective parameter:

      .. parsed-literal::
         :class: highlight

         nrfutil toolchain-manager install --bundle-id *custom_bundle_ID*

      To check the current configuration setting, use the ``nrfutil toolchain-manager config --show`` command.
      To read more about these commands, use the ``nrfutil toolchain-manager --help`` command.

You can check the versions of the required tools and Python dependencies on the :ref:`Requirements reference page <requirements_toolchain>`.

.. _cloning_the_repositories_win:
.. _cloning_the_repositories:

.. rst-class:: numbered-step

Get the |NCS| code
******************

Every |NCS| release consists of a combination of :ref:`ncs_git_intro` repositories at different versions and revisions, managed together by :ref:`ncs_west_intro`.
The revision of each of those repositories is determined by the current revision of the main (or :ref:`manifest <zephyr:west-manifests>`) repository, `sdk-nrf`_.
Simply put, you can work with the following versions of the |NCS|:

.. list-table::
   :header-rows: 1

   * - |NCS| version
     - Required identifier of the revision
     - Where to find the identifier
   * - Specific release (recommended)
     - Release tag (for example, |release_tt|)
     - :ref:`Release_notes` of the release
   * - :ref:`Development tag <dm-revisions>`
     - Development tag (for example, ``v1.9.2-dev1``)
     - :ref:`Changelog <release_notes>` of the tag
   * - Branch
     - Branch name (for example, ``main``)
     - `sdk-nrf`_ repository

.. note::
   Unless you are familiar with the :ref:`development process <dev-model>`, you should always work with a specific release of the |NCS|.

For more information about the repository and development model, see the :ref:`dm_code_base` page.

.. tabs::

   .. group-tab:: nRF Connect for Visual Studio Code

      To clone the |NCS| code, complete the following steps:

      1. Open the nRF Connect extension in |VSC| by clicking its icon in the :guilabel:`Activity Bar`.
      #. In the extension's :guilabel:`Welcome View`, click on :guilabel:`Manage SDKs`.
         The list of actions appears in the |VSC|'s quick pick.
      #. Click :guilabel:`Install SDK`.
         The list of available SDK versions appears in the |VSC|'s quick pick.
      #. Select the SDK version to install.
         |install_latest_version|

      The SDK installation starts and it can take several minutes.

   .. group-tab:: Command line

      To clone the repositories, complete the following steps:

      1. On the command line, open the directory :file:`ncs`.
         By default, this is one level up from the location where you installed the toolchain.
         This directory will hold all |NCS| repositories.

      #. Start the toolchain environment for your operating system using the following command:

         .. tabs::

            .. tab:: Windows

               .. code-block:: console

                  nrfutil toolchain-manager launch --terminal

            .. tab:: Linux

               .. code-block:: console

                  nrfutil toolchain-manager launch --shell

            .. tab:: macOS

               .. code-block:: console

                  nrfutil toolchain-manager launch --shell

         ..

      #. Determine the identifier of the revision of the |NCS| you want to work with.
         See the table above for more information.
         |install_latest_version|

      #. Initialize west with the revision of the |NCS| that you want to check out, replacing *nRFConnectSDK_revision* with the identifier:

         .. parsed-literal::
            :class: highlight

            west init -m https\://github.com/nrfconnect/sdk-nrf --mr *nRFConnectSDK_revision*

         For example:

         * **Specific release:** To check out the |release| release, enter the following command:

           .. parsed-literal::
              :class: highlight

              west init -m https\://github.com/nrfconnect/sdk-nrf --mr |release|

         * **Development tag:** To check out the ``v1.9.2-dev1`` tag, enter the following command:

           .. code-block:: console

              west init -m https://github.com/nrfconnect/sdk-nrf --mr v1.9.2-dev1

         * **Branch**: To check out the ``main`` branch that includes the latest state of development, enter the following command:

           .. code-block:: console

              west init -m https://github.com/nrfconnect/sdk-nrf --mr main

         This will clone the manifest repository `sdk-nrf`_ into :file:`nrf`.

         Initializing west with a specific revision of the :ref:`manifest file <zephyr:west-manifest-files>` does not lock your repositories to this version.
         Checking out a different branch or tag in the `sdk-nrf`_ repository and running ``west update`` changes the version of the |NCS| that you work with.

         .. west-error-start

         .. note::

            If you get an error message when running west, :ref:`update west <west_update>` to the latest version.
            See :ref:`zephyr:west-troubleshooting` in the Zephyr documentation for more information.

         .. west-error-end

      #. Enter the following command to clone the project repositories:

         .. code-block:: console

            west update

         Depending on your connection, this might take some time.
      #. Export a :ref:`Zephyr CMake package <zephyr:cmake_pkg>`.
         This allows CMake to automatically load the boilerplate code required for building |NCS| applications:

         .. code-block:: console

            west zephyr-export

..

If you used the default locations (:file:`C:\ncs` on Windows, :file:`<home>/ncs` on Linux and MacOS), your directory structure now looks similar to this:

.. code-block:: none

   ncs
   ├─── toolchains
   │  └─── <toolchain-installation>
   └─── <west-workspace>
      ├─── .west
      ├─── bootloader
      ├─── modules
      ├─── nrf
      ├─── nrfxlib
      ├─── zephyr
      └─── ...

In this simplified structure preview, *<home>* corresponds to :file:`ncs/` and *<toolchain-installation>* and *<west-workspace>* correspond to the version names you installed.
There are also additional directories, and the structure might change over time, for example if you later :ref:`change the state of development to a different revision <updating_repos>`.
The full set of repositories and directories is defined in the :ref:`manifest file <zephyr:west-manifest-files>` (`see the file in the repository <west manifest file_>`_).

.. _build_environment_cli:

Set up the command-line build environment
*****************************************

.. note::

   This step is only required when working on command line with freestanding applications.

In addition to the steps mentioned above, if you want to build and program your application from the command line, you have to set up your command-line build environment by defining the required environment variables every time you open a new command-line or terminal window.
See :ref:`zephyr:env_vars_important` in the Zephyr documentation for more information about the various relevant environment variables.

Define the required environment variables as follows, depending on your operating system:

.. tabs::

   .. tab:: Windows

      Complete the following steps:

      1. Navigate to the :file:`ncs` directory.
      #. Open the directory for your |NCS| version.
      #. Run the following command in a terminal window:

         .. code-block:: console

            zephyr\zephyr-env.cmd

      If you need to define additional environment variables, create the file :file:`%userprofile%/zephyrrc.cmd` and add the variables there.
      This file is loaded automatically when you run the above command.
      See :ref:`Zephyr documentation about using zephyrrc files <zephyr:env_vars_zephyrrc>` for more information.

   .. tab:: Linux

      Complete the following steps:

      1. Navigate to the :file:`ncs` directory.
      #. Open the directory for your |NCS| version.
      #. Run the following command in a terminal window:

         .. code-block:: console

            source zephyr/zephyr-env.sh

      If you need to define additional environment variables, create the file :file:`~/.zephyrrc` and add the variables there.
      This file is loaded automatically when you run the above command.
      See :ref:`Zephyr documentation about using zephyrrc files <zephyr:env_vars_zephyrrc>` for more information.

   .. tab:: macOS

      Complete the following steps:

      1. Navigate to the :file:`ncs` directory.
      #. Open the directory for your |NCS| version.
      #. Run the following command in a terminal window:

         .. code-block:: console

            source zephyr/zephyr-env.sh

      If you need to define additional environment variables, create the file :file:`~/.zephyrrc` and add the variables there.
      This file is loaded automatically when you run the above command.
      See :ref:`Zephyr documentation about using zephyrrc files <zephyr:env_vars_zephyrrc>` for more information.

.. _additional_deps:

System-wide installation
************************

System-wide installation is an alternative to the recommended installation methods using the |nRFVSC| or nRF Util.
It gives you more control over each of the required tools, but requires more familiarity with Zephyr and with each of the tools.

To install the |NCS| system-wide, complete the following steps:

1. Follow the steps in the following sections of Zephyr's :ref:`zephyr:getting_started` to update your operating system and install dependencies:

   * :ref:`zephyr:host_setup`
   * :ref:`zephyr:install-required-tools`

#. Install west.
   Expand the section below to see the commands.

   .. toggle::

      .. note::

         It is easy to run into Python package incompatibilities when installing dependencies at a system or user level.
         This situation can happen, for example, if working on multiple Zephyr versions or other projects using Python on the same machine.

         For this reason, it is suggested to use `Python virtual environments`_.

      .. tabs::

         .. group-tab:: Windows

            .. tabs::

               .. group-tab:: Install within virtual environment

                  #. Create a new virtual environment:

                     .. code-block:: bat

                        cd %HOMEPATH%
                        python -m venv ncs/.venv

                  #. Activate the virtual environment:

                     .. code-block:: bat

                        ncs\.venv\Scripts\activate.bat

                     Once activated your shell will be prefixed with ``(.venv)``.
                     The virtual environment can be deactivated at any time by running ``deactivate``.

                     .. note::

                        Remember to activate the virtual environment every time you start working.

                  #. Install west:

                     .. code-block:: bat

                        pip3 install west

               .. group-tab:: Install globally

                  #. Install west:

                     .. code-block:: bat

                        pip3 install -U west

         .. group-tab:: Linux

            .. tabs::

               .. group-tab:: Install within virtual environment

                  #. Use ``apt`` to install Python ``venv`` package:

                     .. code-block:: bash

                        sudo apt install python3-venv

                  #. Create a new virtual environment:

                     .. code-block:: bash

                        python3 -m venv ~/ncs/.venv

                  #. Activate the virtual environment:

                     .. code-block:: bash

                        source ~/ncs/.venv/bin/activate

                     Once activated, your shell will be prefixed with ``(.venv)``.
                     The virtual environment can be deactivated at any time by running ``deactivate``.

                     .. note::

                        Remember to activate the virtual environment every time you start working.

                  #. Install west:

                     .. code-block:: bash

                        pip3 install west

               .. group-tab:: Install globally

                  #. Install west and make sure :file:`~/.local/bin` is on your :envvar:`PATH` :ref:`environment variable <env_vars>`:

                     .. code-block:: bash

                        pip3 install --user -U west
                        echo 'export PATH=~/.local/bin:"$PATH"' >> ~/.bashrc
                        source ~/.bashrc

         .. group-tab:: macOS

            .. tabs::

               .. group-tab:: Install within virtual environment

                  #. Create a new virtual environment:

                     .. code-block:: bash

                        python3 -m venv ~/ncs/.venv

                  #. Activate the virtual environment:

                     .. code-block:: bash

                        source ~/ncs/.venv/bin/activate

                     Once activated, your shell will be prefixed with ``(.venv)``.
                     The virtual environment can be deactivated at any time by running ``deactivate``.

                     .. note::

                        Remember to activate the virtual environment every time you start working.

                  #. Install west:

                     .. code-block:: bash

                        pip3 install west

               .. group-tab:: Install globally

                  #. Install west:

                     .. code-block:: bash

                        pip3 install -U west

            ..

      ..

   ..

#. Get the |NCS| code as described in :ref:`cloning_the_repositories` for the command line.
   (You can skip step 2.)
   When you first install the |NCS|, it is recommended to install the latest released version of the SDK.
#. Install the Python dependencies.
   Expand the section below to see the commands.

   .. toggle::

      .. note::

         You might run into Python package incompatibilities when installing dependencies at a system or user level.
         For this reason, it is suggested to use `Python virtual environments`_.

      .. tabs::

         .. group-tab:: Windows

            #. Enter the following commands in a ``cmd.exe`` terminal window in the :file:`ncs` folder:

               .. code-block:: bash

                  pip3 install -r zephyr/scripts/requirements.txt
                  pip3 install -r nrf/scripts/requirements.txt
                  pip3 install -r bootloader/mcuboot/scripts/requirements.txt

         .. group-tab:: Linux

            #. Enter the following commands in a terminal window in the :file:`ncs` folder:

               .. code-block:: bash

                  pip3 install -r zephyr/scripts/requirements.txt
                  pip3 install -r nrf/scripts/requirements.txt
                  pip3 install -r bootloader/mcuboot/scripts/requirements.txt

         .. group-tab:: macOS

            #. Enter the following commands in a terminal window in the :file:`ncs` folder:

               .. code-block:: bash

                  pip3 install -r zephyr/scripts/requirements.txt
                  pip3 install -r nrf/scripts/requirements.txt
                  pip3 install -r bootloader/mcuboot/scripts/requirements.txt

      ..

   ..

#. Follow the steps in the "Install the Zephyr SDK" section in Zephyr's :ref:`zephyr:getting_started` to install the Zephyr SDK.
#. Depending on your preferred development environment:

   * If you want to work with |VSC|, install the |nRFVSC| (the default IDE for the |NCS|).
   * If you want to work from command line, :ref:`build_environment_cli`.

#. If you want to build `Matter`_ applications, additionally install the `GN`_ meta-build system.
   This system generates the Ninja files that the |NCS| uses for Matter.
   See :ref:`manual_installation_gn` for more information.

.. _gs_app_tcm:
.. _gs_assistant:
.. _auto_installation_tcm_setup:

Installation with Toolchain Manager
***********************************

.. note::

   The Toolchain Manager installation is recommended only when using the nRF54H20 DK with the |NCS| v2.7.0.
   When using any other DK, the Toolchain Manager installation is recommended for the |NCS| v1.9.x and earlier versions.

.. toggle::

   Toolchain Manager is a tool available from `nRF Connect for Desktop`_, a cross-platform tool that provides different applications that simplify installing the |NCS|.
   Both the tool and the application are available for Windows, Linux, and macOS.

   To install the toolchain and the SDK using the Toolchain Manager app, complete the following steps:

   1. Install Toolchain Manager:

      a. `Download nRF Connect for Desktop`_ for your operating system.
      #. Install and run the tool on your machine.
      #. In the **APPS** section, click :guilabel:`Install` next to Toolchain Manager.

      The app is installed on your machine, and the :guilabel:`Install` button changes to :guilabel:`Open`.

   #. Install the |NCS| source code:

      a. Open Toolchain Manager in nRF Connect for Desktop.

         .. figure:: images/gs-assistant_tm.png
            :alt: The Toolchain Manager window

            The Toolchain Manager window

      #. Click :guilabel:`SETTINGS` in the navigation bar to specify where you want to install the |NCS|.
      #. In :guilabel:`SDK ENVIRONMENTS`, click the :guilabel:`Install` button next to the |NCS| version that you want to install.
         |install_latest_version|

         The |NCS| version of your choice is installed on your machine.
         The :guilabel:`Install` button changes to :guilabel:`Open VS Code`.

   #. Set up the preferred building method:

      .. tabs::

         .. tab:: nRF Connect for Visual Studio Code

            To build on the |nRFVSC|, complete the following steps:

            a. In Toolchain Manager, click the :guilabel:`Open VS Code` button.

               A notification appears with a list of missing extensions that you need to install, including those from the `nRF Connect for Visual Studio Code`_ extension pack.
            #. Click **Install missing extensions**.
            #. Once the extensions are installed, click **Open VS Code** button again.

            You can then follow the instructions in :ref:`creating_vsc`.

         .. tab:: Command line

            To build on the command line, complete the following steps:

            1. With admin permissions enabled, download and install the `nRF Command Line Tools`_.
            #. Restart the Toolchain Manager application.
            #. Click the dropdown menu for the installed nRF Connect SDK version.

               .. figure:: images/gs-assistant_tm_dropdown.png
                  :alt: The Toolchain Manager dropdown menu for the installed nRF Connect SDK version, on Windows

                  The Toolchain Manager dropdown menu options (Windows)

            #. Select the option to open a terminal window:

               * Windows: :guilabel:`Open command prompt`
               * Linux and macOS: :guilabel:`Open Terminal`

            You can then follow the instructions in :ref:`creating_cmd`.

            .. note::
                You can use the :guilabel:`Generate environment script` button to create a script that lets you switch between different |NCS| versions.
                After you create the script, rename it and put it in a directory in your system :envvar:`PATH`.
                This works the same way as the ``--as-script`` flag for nRF Util's `Toolchain Manager command`_ (``nrfutil toolchain-manager env --as-script``).

.. |install_latest_version| replace:: When you first install the |NCS|, it is recommended to install the latest released versions of the SDK and the toolchain.
