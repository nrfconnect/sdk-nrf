.. _gs_installing:

Installing manually
###################

.. contents::
   :local:
   :depth: 2

The recommended way to get started with the |NCS| is to use nRF Connect for Desktop.
See the :ref:`gs_assistant` page for information about how to install automatically.

.. note::
   If you use nRF Connect for Desktop to install the |NCS|, you can skip this section of the documentation.
   If you prefer to install the toolchain manually, see the following documentation for instructions.

To manually install the |NCS|, you must install all required tools and clone the |NCS| repositories.
See the following sections for detailed instructions.

If you already have your system set up to work with Zephyr OS based on Zephyr's :ref:`zephyr:getting_started`, you already have some of the requirements for the |NCS| installed.
The only requirement not covered by the installation steps in Zephyr is the :ref:`GN tool <gs_installing_gn>`.
This tool is needed only for :ref:`ug_matter` applications.

Before you start setting up the toolchain, install available updates for your operating system.
See :ref:`gs_recommended_versions` for information on the supported operating systems and Zephyr features.

.. _gs_installing_tools:

.. rst-class:: numbered-step

Install the required tools
**************************

The installation process is different depending on your operating system.

.. note::
      You will be asked to reboot after installing some of the tools.
      You can skip these notifications and reboot only once after you complete the installation of all tools.

.. tabs::

   .. group-tab:: Windows

      The recommended way for installing the required tools on Windows is to use `Chocolatey`_, a package manager for Windows.
      Chocolatey installs the tools so that you can use them from a Windows command-line window.

      To install the required tools, complete the following steps:

      .. ncs-include:: develop/getting_started/index.rst
         :docset: zephyr
         :dedent: 6
         :start-after: .. _install_dependencies_windows:
         :end-before: #. Close the window and open a new
      Ensure that these dependencies are installed with their versions as specified in the :ref:`Required tools table <req_tools_table>`.

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
      Also see :ref:`zephyr:mac-setup-alts` in the Zephyr documentation for additional information.

..

.. _gs_installing_gn:

In addition to these required tools, install the `GN`_ meta-build system if you are interested in building `Matter`_ applications.
This system generates the Ninja files that the |NCS| uses.

.. tabs::

   .. group-tab:: Windows

      To install the GN tool, complete the following steps:

      1. Download the latest version of the GN binary archive for Windows from the `GN website`_.
      2. Extract the :file:`zip` archive.
      3. Check that the GN tool is added to your :envvar:`PATH` environment variable.
         See :ref:`zephyr:env_vars` for instructions if needed.

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

.. rst-class:: numbered-step

Install west
************

To manage the combination of repositories and versions, the |NCS| uses :ref:`Zephyr's west <zephyr:west>`.

To install west, reopen the command prompt window to ensure that Python is initialized and complete the following step:

.. tabs::

   .. group-tab:: Windows

      Enter the following command in a command-line window:

      .. parsed-literal::
         :class: highlight

         pip3 install west

   .. group-tab:: Linux

      Enter the following command in a terminal window:

      .. parsed-literal::
         :class: highlight

         pip3 install --user west

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

For more information about the repository and development model, see the :ref:`development model section <dev-model>`.

To clone the repositories, complete the following steps:

1. Create a folder named :file:`ncs`.
   This folder will hold all |NCS| repositories.
#. Determine the identifier of the revision of the |NCS| you want to work with.
   The recommended way is to work with a specific release.

   * To work with a specific release, the identifier is the corresponding tag (for example, |release_tt|).
     You can find the tag in the :ref:`release_notes` of the release.
   * To work with a development tag, the identifier is the corresponding tag (for example, ``v1.2.99-dev1``)
   * To work with a branch, the identifier is the branch name (for example, ``main`` to work with the latest state of development).
   * To work with a specific state, the identifier is the SHA (for example, ``224bee9055d986fe2677149b8cbda0ff10650a6e``).

#. From the command line, initialize west with the revision of the |NCS| that you want to check out, replacing *NCS_revision* with the identifier:

   .. parsed-literal::
      :class: highlight

      west init -m https\://github.com/nrfconnect/sdk-nrf --mr *NCS_revision*

   For example, to check out the |release| release, enter the following command:

   .. parsed-literal::
      :class: highlight

      west init -m https\://github.com/nrfconnect/sdk-nrf --mr |release|

   To check out the latest state of development, enter the following command::

     west init -m https://github.com/nrfconnect/sdk-nrf --mr main

   .. west-error-start

   .. note::

      If you get an error message when running west, :ref:`update west <west_update>` to the latest version.
      See :ref:`zephyr:west-troubleshooting` if you need more information.

      .. west-error-end

      Initializing west with a specific revision of the manifest file does not lock your repositories to this version.
      Checking out a different branch or tag in the `sdk-nrf`_ repository and running ``west update``  changes the version of the |NCS| that you work with.

   This will clone the manifest repository `sdk-nrf`_ into :file:`nrf`.

#. Enter the following command to clone the project repositories::

      west update

   Depending on your connection, this might take some time.
#. Export a :ref:`Zephyr CMake package <zephyr:cmake_pkg>`.
   This allows CMake to automatically load the boilerplate code required for building |NCS| applications::

      west zephyr-export

Your directory structure now looks similar to this::

   ncs
    |___ .west
    |___ bootloader
    |___ modules
    |___ nrf
    |___ nrfxlib
    |___ zephyr
    |___ ...


Note that there are additional folders, and that the structure might change.
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

.. rst-class:: numbered-step

Install a Toolchain
*******************

A toolchain provides a compiler, assembler, linker, and other programs required to build Zephyr applications.

.. ncs-include:: develop/getting_started/index.rst
   :docset: zephyr
   :dedent: 0
   :start-after: to build Zephyr applications.
   :end-before: .. _getting_started_run_sample:

.. rst-class:: numbered-step

Install |VSC|
*************

You can install the |VSC| to open and compile projects in the |NCS|.

.. _installing_vsc:

|vsc_extension_instructions|

.. vsc_mig_note_start

.. note::

   If you are building the application or sample using SEGGER Embedded Studio IDE or on the command line and want to migrate to |VSC|, follow the instructions in the `migrating from other IDEs to VS Code <Migrating IDE_>`_ documentation.

.. vsc_mig_note_end

For instructions specifically for installing, see `Installing using Visual Studio Code`_.

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
