.. _install_ncs:
.. _auto_installation:
.. _gs_installing:
.. _installing_vsc:
.. _gs_installing_toolchain:

Installing the |NCS|
####################

.. contents::
   :local:
   :depth: 2

The recommended method to install the |NCS| is to use the nRF Connect extension in |VSC| and install the latest stable release of the |NCS|.

Complete the following steps:

1. Install available updates for your operating system to make sure it supports the |NCS| firmware.

   .. toggle:: Supported operating systems
      :custom-hints:

      .. include:: ./recommended_versions.rst
         :start-after: os_table_start
         :end-before: os_table_end

2. Install the latest version of |VSC| for your operating system from the `Visual Studio Code download page`_.
3. Click the following button to install the latest version of |nRFVSC|:

   .. ncs-install-vscode-ext::

4. Click the following button to install the latest stable |NCS| release (|release|):

   .. ncs-install-from-vscode::

   This starts the installation of the |NCS| source code and the |NCS| :term:`toolchain`.
   It can take several minutes.
   You can follow the progress in the notification that appears in |VSC|.

   .. toggle:: What latest stable release means
      :custom-hints:

      The latest stable release is the latest release tag in the `sdk-nrf`_ repository that is not a release candidate (``*-rc``) or a preview tag (``*-preview``).
      For each version of the |NCS|, Nordic Semiconductor provides a dedicated :ref:`requirements_toolchain`.

      Unless you are familiar with the :ref:`development process <dev-model>`, you should always work with a specific, stable release of the |NCS|.
      For more information about the development and release model, see the :ref:`dm_code_base` and :ref:`ncs_release_model` pages, respectively.

   .. toggle:: Installing manually
      :custom-hints:

      If you prefer, you can install the toolchain and the SDK in |VSC| manually.
      Follow the steps in the `extension get started guides <How to install the extension_>`_.
      For a more guided installation approach, see the `Installing nRF Connect SDK and VS Code`_ exercise of the `nRF Connect SDK Fundamentals course`_ on Nordic Developer Academy.

After the installation is complete, the extension's :guilabel:`Welcome View` is updated to feature :guilabel:`Manage toolchains` and :guilabel:`Manage SDKs` menus.
You can use these menus to install other versions of the |NCS| and toolchain, either together or separately.

You now have everything that is required by Zephyr's :ref:`zephyr:getting_started` together with additional tools and Python dependencies that the |NCS| uses.

With the default locations to install the SDK code (:file:`C:/ncs` on Windows, :file:`~/ncs/` on Linux, and :file:`/opt/nordic/ncs/` on macOS) and its toolchain (:file:`C:/ncs/toolchains` on Windows, :file:`~/ncs/toolchains/` on Linux, and the non-modifiable :file:`/opt/nordic/ncs/toolchains/` on macOS), your directory structure now looks similar to this:

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

In this simplified structure preview, *<toolchain-installation>* corresponds to the toolchain version (most commonly, a SHA) and *<west-workspace>* corresponds to the SDK version name.
There are also additional directories, and the structure might change over time, for example if you later :ref:`change the state of development to a different revision <updating_repos>`.
The full set of repositories and directories is defined in the :ref:`manifest file <zephyr:west-manifest-files>` (`see the file in the repository <west manifest file_>`_).

.. _gs_installing_tools:
.. _cloning_the_repositories_win:
.. _cloning_the_repositories:

Alternative method: Installing using command line
*************************************************

As an alternative to using the recommended |nRFVSC|, you can also install the |NCS| using command line and :ref:`requirements_nrf_util`.

Complete the following steps:

1. Install available updates for your :ref:`operating system <supported_OS>` to make sure it supports the |NCS| firmware.

   .. include:: ./recommended_versions.rst
      :start-after: os_table_start
      :end-before: os_table_end

2. Install the following required software tools:

   .. tabs::

      .. tab:: Windows

         * The |jlink_ver| of SEGGER J-Link.
           Download it from the `J-Link Software and Documentation Pack`_ page and `install it manually together with SEGGER USB Driver for J-Link <nRF Util prerequisites_>`_.
           The SEGGER USB Driver for J-Link is required for support of older Nordic Semiconductor devices.
         * `Download the executable <nRF Util Downloads_>`_ for the latest version of nRF Util.

           .. note::
              After downloading the nRF Util executable, move it to a directory that is in the system :envvar:`PATH`.

      .. tab:: Linux

         * The |jlink_ver| of SEGGER J-Link.
           Download it from the `J-Link Software and Documentation Pack`_ page.
         * `Download the executable <nRF Util Downloads_>`_ for the latest version of nRF Util.

           .. note::
              After downloading the nRF Util executable, move it to a directory that is in the system :envvar:`PATH`.
              The downloaded file also needs to be given execute permission by typing `chmod +x nrfutil` or by checking the checkbox in the file properties.

         * The `nrf-udev`_ module with udev rules required to access USB ports on Nordic Semiconductor devices and program the firmware.

      .. tab:: macOS

         * The |jlink_ver| of SEGGER J-Link.
           Download it from the `J-Link Software and Documentation Pack`_ page.
         * `Download the executable <nRF Util Downloads_>`_ for the latest version of nRF Util.

           .. note::
              After downloading the nRF Util executable, move it to a directory that is in the system :envvar:`PATH`.
              The downloaded file also needs to be given execute permission by typing `chmod +x nrfutil` or by checking the checkbox in the file properties.

3. Install the latest stable |NCS| code and toolchain by completing the following steps:

   .. toggle:: What latest stable release means
      :custom-hints:

      The latest stable release is the latest release tag in the `sdk-nrf`_ repository that is not a release candidate (``*-rc``) or a preview tag (``*-preview``).
      For each version of the |NCS|, Nordic Semiconductor provides a dedicated :ref:`requirements_toolchain`.

      Unless you are familiar with the :ref:`development process <dev-model>`, you should always work with a specific, stable release of the |NCS|.
      For more information about the development and release model, see the :ref:`dm_code_base` and :ref:`ncs_release_model` pages, respectively.

   1. Open a terminal window.
   #. Remove the lock on the nRF Util installation to be able to install other nRF Util commands.
      See `Locking nRF Util home directory`_ in the tool documentation for more information.
   #. Run the following command to install the nRF Util's ``sdk-manager`` command:

      .. code-block:: console

         nrfutil install sdk-manager

   #. Run the following command to list the available installations:

      .. code-block:: console

         nrfutil sdk-manager search

      The versions from this list correspond to the |NCS| versions and will be *version* in the following step.
   #. Run the following command to install the SDK and the toolchain for the SDK version of your choice:

      .. parsed-literal::
         :class: highlight

         nrfutil sdk-manager install *version*

      For example:

      .. parsed-literal::
         :class: highlight

         nrfutil sdk-manager install |release|

      This example command installs both the SDK code and the toolchain for the |NCS| |release|.
      |install_latest_version|

      .. note::
         If you plan to work with a specific branch, use the command for installing the latest |NCS| release (|release|) to install the latest toolchain.
         Then, you must use the ``west init`` command several steps below to get the code of the desired branch.
         This is because the ``sdk-manager`` command only supports installing specific release versions.

      The ``sdk-manager`` command installs the SDK by default at :file:`C:/ncs/` on Windows and at :file:`~/ncs/` on Linux, and the toolchain in the :file:`toolchains` subdirectory.
      The SDK installation location can be modified, as explained in the `command documentation <sdk-manager Configuration settings_>`_.
      On macOS, :file:`/opt/nordic/ncs/` and :file:`/opt/nordic/ncs/toolchains` are used and no other locations are allowed.
      The :file:`ncs` directory holds all |NCS| repositories.

      If you have received a custom URL or a toolchain bundle ID for installing the toolchain, you can use dedicated commands to provide it.
      Expand the section below to see the commands.

      .. toggle::

         If you have received a custom URL for installing the toolchain, you can use the following commands to set it as default, replacing the respective parameters:

         .. parsed-literal::
            :class: highlight

            nrfutil sdk-manager config toolchain-index add *index-name* *custom_toolchain_URL*
            nrfutil sdk-manager config toolchain-index set *index-name*

         If you have received a custom bundle ID for installing a specific toolchain version, you can use the following commands to provide it, replacing the respective parameter:

         .. parsed-literal::
            :class: highlight

            nrfutil sdk-manager toolchain install --toolchain-bundle-id *custom_bundle_ID*

         To learn more about ``sdk-manager`` commands, use the ``nrfutil sdk-manager --help`` command, or see the `command documentation <sdk-manager command_>`_.

   #. Start the toolchain environment for your operating system using the following command pattern, with ``--ncs-version`` corresponding to the |NCS| version you have installed:

      .. tabs::

         .. tab:: Windows

            .. parsed-literal::
               :class: highlight

               nrfutil sdk-manager toolchain launch --ncs-version *version* --terminal

            For example:

            .. parsed-literal::
               :class: highlight

               nrfutil sdk-manager toolchain launch --ncs-version |release| --terminal

            This example command starts the toolchain environment for the |NCS| |release|.

         .. tab:: Linux

            .. parsed-literal::
               :class: highlight

               nrfutil sdk-manager toolchain launch --ncs-version *version* --shell

            For example:

            .. parsed-literal::
               :class: highlight

               nrfutil sdk-manager toolchain launch --ncs-version |release| --shell

            This example command starts the toolchain environment for the |NCS| |release|.

         .. tab:: macOS

            .. parsed-literal::
               :class: highlight

               nrfutil sdk-manager toolchain launch --ncs-version *version* --shell

            For example:

            .. parsed-literal::
               :class: highlight

               nrfutil sdk-manager toolchain launch --ncs-version |release| --shell

            This example command starts the toolchain environment for the |NCS| |release|.

      ..

      .. note::

         You can also use other options instead of ``--ncs-version``.
         See the `sdk-manager command documentation <sdk-manager Starting and inspecting the toolchain environment_>`_ for more information.

   #. Determine the identifier of the revision of the |NCS| you want to work with.
      See the table above for more information.
      |install_latest_version|

   #. Initialize west with the revision of the |NCS| that you want to check out, replacing the required parameters:

      .. parsed-literal::
         :class: highlight

         west init -m https\://github.com/nrfconnect/sdk-nrf --mr *nRFConnectSDK_revision* *nRFConnectSDK_revision_workspace_dir*

      In this command:

      - The first *nRFConnectSDK_revision* identifies the revision of the |NCS|.
      - The second *nRFConnectSDK_revision_workspace_dir* is the name of the workspace directory that will be created by west.
         For this installation method, it should be the same as the *nRFConnectSDK_revision*.

      The command creates the *nRFConnectSDK_revision* subdirectory and checks out the given revision of the |NCS| inside it.
      For example:

      * **Specific release:** To check out the |release| release, enter the following command:

         .. parsed-literal::
            :class: highlight

            west init -m https\://github.com/nrfconnect/sdk-nrf --mr |release| |release|

      * **Git tag:** To check out the ``v2.8.0-rc1`` tag, enter the following command:

         .. code-block:: console

            west init -m https://github.com/nrfconnect/sdk-nrf --mr v2.8.0-rc1 v2.8.0-rc1

      * **Branch**: To check out the ``main`` branch that includes the latest state of development, enter the following command:

         .. code-block:: console

            west init -m https://github.com/nrfconnect/sdk-nrf --mr main main

      This will clone the manifest repository `sdk-nrf`_ into :file:`nrf`.

      Initializing west with a specific revision of the :ref:`manifest file <zephyr:west-manifest-files>` does not lock your repositories to this version.
      Checking out a different branch or tag in the `sdk-nrf`_ repository and running ``west update`` changes the version of the |NCS| that you work with.

      .. west-error-start

      .. note::

         If you get an error message when running west, :ref:`update west <west_update>` to the latest version.
         See :ref:`zephyr:west-troubleshooting` in the Zephyr documentation for more information.

      .. west-error-end

   #. Enter the *nRFConnectSDK_revision* subdirectory and run the following command to clone the project repositories:

      .. code-block:: console

         west update

      Depending on your connection, this might take some time.
   #. Export a :ref:`Zephyr CMake package <zephyr:cmake_pkg>`.
      This allows CMake to automatically load the boilerplate code required for building |NCS| applications:

      .. code-block:: console

         west zephyr-export

   With the default locations to install the SDK code (:file:`C:/ncs` on Windows, :file:`~/ncs/` on Linux, and :file:`/opt/nordic/ncs/` on macOS) and its toolchain (:file:`C:/ncs/toolchains` on Windows, :file:`~/ncs/toolchains/` on Linux, and the non-modifiable :file:`/opt/nordic/ncs/toolchains/` on macOS), your directory structure now looks similar to this:

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

   In this simplified structure preview, *<toolchain-installation>* corresponds to the toolchain version (most commonly, a SHA) and *<west-workspace>* corresponds to the SDK version name.
   There are also additional directories, and the structure might change over time, for example if you later :ref:`change the state of development to a different revision <updating_repos>`.
   The full set of repositories and directories is defined in the :ref:`manifest file <zephyr:west-manifest-files>` (`see the file in the repository <west manifest file_>`_).

.. _build_environment_cli:

Set up the command-line build environment
=========================================

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

Alternative method: System-wide installation
********************************************

System-wide installation is an alternative to the recommended installation methods using |nRFVSC| or nRF Util.
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

#. Enter the :file:`ncs` directory you created when setting up the virtual environment:

   .. tabs::

      .. group-tab:: Windows

         .. code-block:: console

            cd %HOMEPATH%/ncs

      .. group-tab:: Linux

         .. code-block:: console

            cd ~/ncs

      .. group-tab:: macOS

         .. code-block:: console

            cd ~/ncs

#. Get the |NCS| code.
   When you first install the |NCS|, it is recommended to install the latest released version of the SDK.
   Run the following command, where *nRFConnectSDK_revision* is the revision of the |NCS| you want to get the code of:

   .. code-block:: console

      west init -m https://github.com/nrfconnect/sdk-nrf --mr *nRFConnectSDK_revision*

   The command initializes a west workspace in the :file:`ncs` directory and checks out the given revision of the |NCS| `sdk-nrf`_ in the :file:`nrf` subdirectory.
   For example:

   * **Specific release:** To check out the |release| release, enter the following command:

      .. parsed-literal::
         :class: highlight

         west init -m https\://github.com/nrfconnect/sdk-nrf --mr |release|

   * **Git tag:** To check out the ``v2.8.0-rc1`` tag, enter the following command:

      .. code-block:: console

         west init -m https://github.com/nrfconnect/sdk-nrf --mr v2.8.0-rc1

   * **Branch**: To check out the ``main`` branch that includes the latest state of development, enter the following command:

      .. code-block:: console

         west init -m https://github.com/nrfconnect/sdk-nrf --mr main

     Alternatively, you can omit the ``--mr`` parameter, in which case west uses ``main`` as the default *nRFConnectSDK_revision*.

#. In the :file:`ncs` directory, run the following command to clone the project repositories:

   .. code-block:: console

      west update

#. Install the Python dependencies.
   Expand the section below to see the commands.

   .. toggle::

      .. note::

         You might run into Python package incompatibilities when installing dependencies at a system or user level.
         For this reason, it is suggested to use `Python virtual environments`_.

      .. tabs::

         .. group-tab:: Windows

            #. In the :file:`ncs` directory, enter the following commands in a ``cmd.exe`` terminal window:

               .. code-block:: bash

                  pip3 install -r zephyr/scripts/requirements.txt
                  pip3 install -r nrf/scripts/requirements.txt
                  pip3 install -r bootloader/mcuboot/scripts/requirements.txt

         .. group-tab:: Linux

            #. In the :file:`ncs` directory, enter the following commands in a terminal window:

               .. code-block:: bash

                  pip3 install -r zephyr/scripts/requirements.txt
                  pip3 install -r nrf/scripts/requirements.txt
                  pip3 install -r bootloader/mcuboot/scripts/requirements.txt

         .. group-tab:: macOS

            #. In the :file:`ncs` directory, enter the following commands in a terminal window:

               .. code-block:: bash

                  pip3 install -r zephyr/scripts/requirements.txt
                  pip3 install -r nrf/scripts/requirements.txt
                  pip3 install -r bootloader/mcuboot/scripts/requirements.txt

      ..

   ..

#. Follow the steps in the "Install the Zephyr SDK" section in Zephyr's :ref:`zephyr:getting_started` to install the Zephyr SDK.
#. Depending on your preferred development environment:

   * If you want to work with |VSC|, install |nRFVSC| (the default IDE for the |NCS|).
   * If you want to work from command line, :ref:`build_environment_cli`.

#. If you want to build `Matter`_ applications, additionally install the `GN`_ meta-build system.
   This system generates the Ninja files that the |NCS| uses for Matter.
   See the `GN tool`_ page for more information.
