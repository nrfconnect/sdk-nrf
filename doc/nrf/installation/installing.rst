.. _gs_installing:
.. _manual_installation:

Installing manually
###################

.. contents::
   :local:
   :depth: 2

The recommended way to get started with the |NCS| is to install using nRF Connect for Desktop, which is described on the :ref:`auto_installation` page.
However, if you prefer to install the toolchain manually, follow the instructions described below.

To manually install the |NCS|, you must first install all the required tools and clone the |NCS| repositories.

The |NCS| :term:`toolchain` includes the Zephyr SDK and then adds on top of it tools and modules required to build |NCS| samples and applications.
If you already have your system set up to work with Zephyr OS, based on Zephyr's :ref:`zephyr:getting_started`, it means you have most of the requirements for the |NCS| installed.
However, you still need to install a set of additional tools, including Python dependencies and the |nRFVSC| (the default IDE for the |NCS|).
You might also need the :ref:`GN tool <manual_installation_gn>` if you are interested in creating :ref:`ug_matter` applications.

.. _manual_installation_update_os:

.. rst-class:: numbered-step

Update your operating system
****************************

Before you start setting up the |NCS| toolchain, install available updates for your operating system.
See :ref:`requirements` for information on the supported operating systems and Zephyr features.

.. _gs_installing_tools:
.. _manual_installation_tools:

.. rst-class:: numbered-step

Install the required tools
**************************

The installation process is different depending on your operating system.

.. note::
      You will be asked to reboot after installing some of the tools.
      You can skip these notifications and reboot only once after you complete the installation of all tools.

.. tabs::

   .. group-tab:: Windows

      The recommended way of installing the required tools on Windows is to use `Chocolatey`_, a package manager for Windows.
      Chocolatey installs the tools so that you can use them from a Windows command-line window.

      To install the required tools, complete the following steps:

      .. ncs-include:: develop/getting_started/index.rst
         :docset: zephyr
         :dedent: 6
         :start-after: .. _install_dependencies_windows:
         :end-before: #. Close the window and open a new

      Ensure that these dependencies are installed with their versions as specified in the :ref:`Required tools table <req_tools_table>`.
      To check the list of installed packages and their versions, run the following command:

       .. parsed-literal::
          :class: highlight

           choco list -lo

   .. group-tab:: Linux

      To install the required tools on Ubuntu, complete the following steps:

      .. ncs-include:: develop/getting_started/index.rst
         :docset: zephyr
         :dedent: 6
         :start-after: .. _install_dependencies_ubuntu:
         :end-before: Check those against the versions in the table in the beginning of this section.

      Ensure that these dependencies are installed with their versions as specified in the :ref:`Required tools table <req_tools_table>`.
      Refer to the :ref:`zephyr:installation_linux` page for additional information on updating the dependencies manually.
      If you are using other Linux-based operating systems, see the :ref:`zephyr:linux_requirements` section in the Zephyr documentation.

   .. group-tab:: macOS

      To install the required tools, complete the following steps:

      .. ncs-include:: develop/getting_started/index.rst
         :docset: zephyr
         :dedent: 6
         :start-after: .. _install_dependencies_macos:
         :end-before: group-tab:: Windows

      Ensure that these dependencies are installed with their versions as specified in the :ref:`Required tools table <req_tools_table>`.
      To check the installed versions, run the following command:

       .. parsed-literal::
          :class: highlight

           brew list --versions

      Also see :ref:`zephyr:mac-setup-alts` in the Zephyr documentation for additional information.
..

.. _gs_installing_gn:
.. _manual_installation_gn:

If you want to build `Matter`_ applications, additionally install the `GN`_ meta-build system.
This system generates the Ninja files that the |NCS| uses.

.. tabs::

   .. group-tab:: Windows

      To install the GN tool, complete the following steps:

      1. Download the latest version of the GN binary archive for Windows from the `GN website`_.
      2. Extract the :file:`zip` archive.
      3. Ensure that the GN tool is added to your :envvar:`PATH` environment variable.
         For the exact instructions, see :ref:`zephyr:env_vars`.

   .. group-tab:: Linux

      To install the GN tool, complete the following steps:

      1. Create the directory for the GN tool:

         .. parsed-literal::
            :class: highlight

            mkdir ${HOME}/gn && cd ${HOME}/gn

      #. Download the GN binary archive and extract it by using the following commands:

         .. parsed-literal::
            :class: highlight

            wget -O gn.zip https:\ //chrome-infra-packages.appspot.com/dl/gn/gn/linux-amd64/+/latest
            unzip gn.zip
            rm gn.zip

         The wget tool is installed when installing the required tools on Linux.
      #. Add the location of the GN tool to the system :envvar:`PATH`.
         For example, if you are using ``bash``, run the following commands:

         .. parsed-literal::
            :class: highlight

            echo 'export PATH=${HOME}/gn:"$PATH"' >> ${HOME}/.bashrc
            source ${HOME}/.bashrc

   .. group-tab:: macOS

      To install the GN tool, complete the following steps:

      1. Create the directory for the GN tool:

         .. parsed-literal::
            :class: highlight

            mkdir ${HOME}/gn && cd ${HOME}/gn

      #. Install the wget tool:

         .. parsed-literal::
            :class: highlight

            brew install wget

      #. Download the GN binary archive and extract it by using the following commands:

         * For 64-bit ARM (M1 and M2) host architecture:

         .. parsed-literal::
            :class: highlight

            wget -O gn.zip https:\ //chrome-infra-packages.appspot.com/dl/gn/gn/mac-arm64/+/latest
            unzip gn.zip
            rm gn.zip

         * For 64-bit AMD (Intel) host architecture:

         .. parsed-literal::
            :class: highlight

            wget -O gn.zip https:\ //chrome-infra-packages.appspot.com/dl/gn/gn/mac-amd64/+/latest
            unzip gn.zip
            rm gn.zip

      #. Add the location of the GN tool to the system :envvar:`PATH`.
         For example, if you are using ``bash``, run the following commands:

         a. Create the :file:`.bash_profile` file if you do not have it already:

            .. parsed-literal::
               :class: highlight

               touch ${HOME}/.bash_profile

         #. Add the location of the GN tool to :file:`.bash_profile`:

            .. parsed-literal::
               :class: highlight

               echo 'export PATH=${HOME}/gn:"$PATH"' >> ${HOME}/.bash_profile
               source ${HOME}/.bash_profile

..

.. _gs_installing_west:
.. _manual_installation_west:

.. rst-class:: numbered-step

Install west
************

To manage the combination of repositories and versions, the |NCS| uses :ref:`Zephyr's west <zephyr:west>`.

To install west, reopen the command prompt window as an administrator to ensure that Python is initialized, and complete the following step:

.. tabs::

   .. group-tab:: Windows

      Enter the following command in a command-line window:

      .. parsed-literal::
         :class: highlight

         pip3 install west

      .. note::
         Ensure the west location is added to the path in environmental variables.

   .. group-tab:: Linux

      Enter the following command in a terminal window:

      .. parsed-literal::
         :class: highlight

         pip3 install --user west
         echo 'export PATH=~/.local/bin:"$PATH"' >> ~/.bashrc
         source ~/.bashrc

   .. group-tab:: macOS

      Enter the following command in a terminal window:

      .. parsed-literal::
         :class: highlight

         pip3 install west

You only need to do this once.

.. _cloning_the_repositories_win:
.. _cloning_the_repositories:

.. rst-class:: numbered-step

Get the |NCS| code
******************

Every |NCS| release consists of a combination of :ref:`Git repositories <ncs_introduction>` at different revisions.
The revision of each of those repositories is determined by the current revision of the main (or manifest) repository, `sdk-nrf`_.

.. note::
   The latest state of development is on the ``main`` branch of the `sdk-nrf`_ repository.
   To ensure a usable state, the `sdk-nrf`_ repository defines the compatible states of the other repositories.
   However, this state is not necessarily tested.
   For a higher degree of quality assurance, check out a tagged release.

   Unless you are familiar with the development process, you should always work with a specific release of the |NCS|.

For more information about the repository and development model, see the :ref:`development model <dev-model>` page.

To clone the repositories, complete the following steps:

1. Create a folder named :file:`ncs`.
   This folder will hold all |NCS| repositories.

#. Determine the identifier of the revision of the |NCS| you want to work with.
   The recommended way is to work with a specific release.

   * To work with a specific release, the identifier is the corresponding tag (for example, |release_tt|).
     You can find the tag in the :ref:`release_notes` of the release.
   * To work with a :ref:`development tag <intro_vers_revs>`, the identifier is the corresponding tag (for example, ``v1.2.99-dev1``)
   * To work with a branch, the identifier is the branch name (for example, ``main`` to work with the latest state of development).
   * To work with a specific state, the identifier is the SHA (for example, ``224bee9055d986fe2677149b8cbda0ff10650a6e``).

#. On the command line, go to the :file:`ncs` folder (``cd ncs``) and initialize west with the revision of the |NCS| that you want to check out, replacing *nRFConnectSDK_revision* with the identifier:

   .. parsed-literal::
      :class: highlight

      west init -m https\://github.com/nrfconnect/sdk-nrf --mr *nRFConnectSDK_revision*

   For example, to check out the |release| release, enter the following command:

   .. parsed-literal::
      :class: highlight

      west init -m https\://github.com/nrfconnect/sdk-nrf --mr |release|

   .. west-error-start

   .. note::

      If you get an error message when running west, :ref:`update west <west_update>` to the latest version.
      See :ref:`zephyr:west-troubleshooting` in the Zephyr documentation for more information.

      .. west-error-end

      Initializing west with a specific revision of the manifest file does not lock your repositories to this version.
      Checking out a different branch or tag in the `sdk-nrf`_ repository and running ``west update`` changes the version of the |NCS| that you work with.

   To check out the latest state of development, enter the following command::

     west init -m https://github.com/nrfconnect/sdk-nrf --mr main

   This will clone the manifest repository `sdk-nrf`_ into :file:`nrf`.

#. Enter the following command to clone the project repositories::

      west update

   Depending on your connection, this might take some time.
#. Export a :ref:`Zephyr CMake package <zephyr:cmake_pkg>`.
   This allows CMake to automatically load the boilerplate code required for building |NCS| applications::

      west zephyr-export

Your directory structure now looks similar to this:

.. code-block::

   ncs
    |___ .west
    |___ bootloader
    |___ modules
    |___ nrf
    |___ nrfxlib
    |___ zephyr
    |___ ...

This is a simplified structure preview.
There are additional folders, and the structure might change over time.
The full set of repositories and folders is defined in the manifest file.

.. _additional_deps:

.. rst-class:: numbered-step

Install additional Python dependencies
**************************************

The |NCS| requires additional Python packages to be installed.

Use the following commands to install the requirements for each repository.

.. tabs::

   .. group-tab:: Windows

      Enter the following command in a command-line window in the :file:`ncs` folder:

        .. parsed-literal::
           :class: highlight

           pip3 install -r zephyr/scripts/requirements.txt
           pip3 install -r nrf/scripts/requirements.txt
           pip3 install -r bootloader/mcuboot/scripts/requirements.txt

   .. group-tab:: Linux

      Enter the following command in a terminal window in the :file:`ncs` folder:

        .. parsed-literal::
           :class: highlight

           pip3 install --user -r zephyr/scripts/requirements.txt
           pip3 install --user -r nrf/scripts/requirements.txt
           pip3 install --user -r bootloader/mcuboot/scripts/requirements.txt

   .. group-tab:: macOS

      Enter the following command in a terminal window in the :file:`ncs` folder:

        .. parsed-literal::
           :class: highlight

           pip3 install -r zephyr/scripts/requirements.txt
           pip3 install -r nrf/scripts/requirements.txt
           pip3 install -r bootloader/mcuboot/scripts/requirements.txt

..

.. _gs_installing_toolchain:
.. _manual_installation_toolchain:

.. rst-class:: numbered-step

Install the Zephyr SDK
**********************

The Zephyr Software Development Kit (SDK) contains toolchains for each of Zephyr's supported architectures.
Each toolchain provides a compiler, assembler, linker, and some, but not all, of the rest of the programs required to build Zephyr applications.
The Zephyr SDK also includes additional host tools, such as custom QEMU and OpenOCD builds.
It is at the base of the |NCS| :term:`toolchain`, which adds on top of it several tools and modules of its own.

.. note::
   When updating the Zephyr SDK, :ref:`verify the Zephyr SDK variables <zephyr:toolchain_zephyr_sdk_update>`.
   Make sure that the ``zephyr`` toolchain is selected, not ``gnuarmemb``.

.. tabs::

   .. group-tab:: Windows

      .. ncs-include:: develop/getting_started/index.rst
         :docset: zephyr
         :dedent: 6
         :start-after:   .. _windows_zephyr_sdk:
         :end-before: .. _getting_started_run_sample:

   .. group-tab:: Linux

      .. ncs-include:: develop/getting_started/index.rst
         :docset: zephyr
         :dedent: 6
         :start-after:   .. _ubuntu_zephyr_sdk:
         :end-before: .. group-tab:: macOS

   .. group-tab:: macOS

      .. ncs-include:: develop/getting_started/index.rst
         :docset: zephyr
         :dedent: 6
         :start-after:  .. _macos_zephyr_sdk:
         :end-before: .. group-tab:: Windows

.. rst-class:: numbered-step

Install |nRFVSC|
****************

To open and compile projects in the |NCS|, install and use the recommended |nRFVSC|.

.. _installing_vsc:

|vsc_extension_description|
For installation and migration instructions, see `How to install the extension`_.
For other instructions related to the |nRFVSC|, see the `nRF Connect for Visual Studio Code`_ documentation site.

.. _build_environment_cli:

Set up the command-line build environment
*****************************************

In addition to |VSC|, you can also build and program your application from the command line.
You have to set up your build environment by defining the required environment variables every time you open a new command-line or terminal window.

See :ref:`zephyr:important-build-vars` for more information about the various relevant environment variables.

Define the required environment variables as follows, depending on your operating system:

.. tabs::

   .. group-tab:: Windows

      Navigate to the :file:`ncs` folder and enter the following command: ``zephyr\zephyr-env.cmd``

      If you need to define additional environment variables, create the file :file:`%userprofile%\zephyrrc.cmd` and add the variables there.
      This file is loaded automatically when you run the above command.
      See :ref:`zephyr:env_vars_zephyrrc` for more information.

   .. group-tab:: Linux

      Navigate to the :file:`ncs` folder and enter the following command: ``source zephyr/zephyr-env.sh``

      If you need to define additional environment variables, create the file :file:`~/.zephyrrc` and add the variables there.
      This file is loaded automatically when you run the above command.
      See :ref:`zephyr:env_vars_zephyrrc` for more information.


   .. group-tab:: macOS

      Navigate to the :file:`ncs` folder and enter the following command: ``source zephyr/zephyr-env.sh``

      If you need to define additional environment variables, create the file :file:`~/.zephyrrc` and add the variables there.
      This file is loaded automatically when you run the above command.
      See :ref:`zephyr:env_vars_zephyrrc` for more information.

You must also make sure that nrfjprog (part of the `nRF Command Line Tools`_) is installed and its path is added to the environment variables.
The west command programs the development kit by using nrfjprog by default.
For more information on nrfjprog, see `Programming SoCs with nrfjprog`_.
