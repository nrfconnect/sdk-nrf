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
You can also skip the `Install the GNU Arm Embedded Toolchain`_ section.

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

      .. ncs-include:: getting_started/index.rst
         :docset: zephyr
         :dedent: 6
         :start-after: .. _install_dependencies_windows:
         :end-before: #. Close the window and open a new

   .. group-tab:: Linux

      To install the required tools on Ubuntu, complete the following steps:

      .. ncs-include:: getting_started/index.rst
         :docset: zephyr
         :dedent: 6
         :start-after: .. _install_dependencies_ubuntu:
         :end-before: Check those against the versions in the table in the beginning of this section.

      Check those against the versions in the :ref:`Required tools table <req_tools_table>`.
      Refer to the :ref:`zephyr:installation_linux` page for additional information on updating the dependencies manually.
      If you are using other Linux-based operating systems, see the :ref:`zephyr:linux_requirements` section in the Zephyr documentation.

      .. note::
         You do not need to install the Zephyr SDK.
         We recommend to install the compiler toolchain separately, as detailed in `Install the GNU Arm Embedded Toolchain`_.

   .. group-tab:: macOS

      To install the required tools, complete the following steps:

      .. ncs-include:: getting_started/index.rst
         :docset: zephyr
         :dedent: 6
         :start-after: .. _install_dependencies_macos:
         :end-before: group-tab:: Windows


      Also see :ref:`zephyr:mac-setup-alts` in the Zephyr documentation for additional information.

..

.. _gs_installing_gn:

In addition to these required tools, install the `GN`_ meta-build system if you are interested in building `Matter`_ (formerly Project Connected Home over IP, Project CHIP) applications.
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

Create a folder named :file:`ncs`.
This folder will hold all |NCS| repositories.

To install west, complete the following step:

.. tabs::

   .. group-tab:: Windows

      Enter the following command in a command-line window in the :file:`ncs` folder:

      .. parsed-literal::
         :class: highlight

         pip3 install west

   .. group-tab:: Linux

      Enter the following command in a terminal window in the :file:`ncs` folder:

      .. parsed-literal::
         :class: highlight

         pip3 install --user west

   .. group-tab:: macOS

      Enter the following command in a terminal window in the :file:`ncs` folder:

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

1. Determine the identifier of the revision of the |NCS| you want to work with.
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

Install the GNU Arm Embedded Toolchain
**************************************

To be able to cross-compile your applications for Arm targets, you must install version 9-2019-q4-major of the `GNU Arm Embedded Toolchain`_.

.. important::
   Make sure to install the toolchain version that is mentioned above.
   Other toolchain versions might not work with this version of the |NCS|.
   Similarly, other versions of the |NCS| might require a different toolchain version.

To set up the toolchain, complete the following steps:

.. _toolchain_setup:

1. Download the `GNU Arm Embedded Toolchain`_ for your operating system.
#. Extract the contents of the root folder of the toolchain into a directory of your choice.
   The recommended folder is :file:`c:/gnuarmemb` on Windows and :file:`~/gnuarmemb` on Linux or macOS.
   Make sure that the folder name does not contain any spaces or special characters.
   By default, the contents are extracted to another folder that corresponds to the GNU Arm Embedded Toolchain version (*version-folder* in the following step).
   For example, :file:`c:/gccarmemb/9_2019-q4-major`, where :file:`9_2019-q4-major` is the *version-folder* name edited to contain no empty spaces.
#. If you want to build and program applications from the command line, define the environment variables for the GNU Arm Embedded Toolchain.
   Depending on your operating system:

    .. tabs::

       .. group-tab:: Windows

          Open a command-line window and enter the commands below.

          If you did not install the toolchain in the recommended folder, change the value of :envvar:`GNUARMEMB_TOOLCHAIN_PATH` to the folder you used and make sure to provide the name of the *version-folder*.

            .. parsed-literal::
               :class: highlight

               set ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
               set GNUARMEMB_TOOLCHAIN_PATH=\ c:\\gnuarmemb\\version-folder

       .. group-tab:: Linux

          Open a terminal window and enter the commands below.

          If you did not install the toolchain in the recommended folder, change the value of :envvar:`GNUARMEMB_TOOLCHAIN_PATH` to the folder you used and make sure to provide the name of the *version-folder*.

            .. parsed-literal::
               :class: highlight

               export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
               export GNUARMEMB_TOOLCHAIN_PATH=\ "~/gnuarmemb/*version-folder*"

       .. group-tab:: macOS

          Open a terminal window and enter the commands below.

          If you did not install the toolchain in the recommended folder, change the value of :envvar:`GNUARMEMB_TOOLCHAIN_PATH` to the folder you used and make sure to provide the name of the *version-folder*.

            .. parsed-literal::
               :class: highlight

               export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
               export GNUARMEMB_TOOLCHAIN_PATH=\ "~/gnuarmemb/*version-folder*"

#. Set the environment variables persistently.
   Depending on your operating system:

    .. tabs::

       .. group-tab:: Windows

          Add the environment variables as system environment variables or define them in the :file:`%userprofile%\zephyrrc.cmd` file as described in :ref:`build_environment_cli`.
          This lets you avoid setting them every time you open a command-line window.

       .. group-tab:: Linux

          Define the environment variables in the :file:`~/.zephyrrc` file as described in :ref:`build_environment_cli`.
          This lets you avoid setting them every time you open a terminal window.

       .. group-tab:: macOS

          Define the environment variables in the :file:`~/.zephyrrc` file as described in :ref:`build_environment_cli`.
          This lets you avoid setting them every time you open a terminal window.

.. rst-class:: numbered-step

Install a build IDE
*******************

You can install either the |VSC| extension or the |SES| (SES) Nordic Edition to open and compile projects in the |NCS|:

.. _installing_vsc:

Visual Studio Code extension
============================

|vsc_extension_instructions|

.. _installing_ses:

|SES| Nordic Edition
====================

You can install the |SES| (SES) Nordic Edition to open and compile projects in the |NCS|.
SES is free of charge for use with Nordic Semiconductor devices.

To install the Nordic Edition, complete the following steps:

1. Download the package for your operating system:

    .. tabs::

       .. group-tab:: Windows

          * `SEGGER Embedded Studio (Nordic Edition) - Windows x86`_
          * `SEGGER Embedded Studio (Nordic Edition) - Windows x64`_

       .. group-tab:: Linux

          * `SEGGER Embedded Studio (Nordic Edition) - Linux x86`_
          * `SEGGER Embedded Studio (Nordic Edition) - Linux x64`_

       .. group-tab:: macOS

          * `SEGGER Embedded Studio (Nordic Edition) - Mac OS x64`_

#. Extract the downloaded package in the directory of your choice.
#. Register and activate a free license.
   |SES| is free of charge for use with Nordic Semiconductor devices, but you still need to request and activate a license.
   Complete the following steps:

    a. Navigate to the :file:`bin` directory.
    #. Run the :file:`emStudio` executable file.
       SES first opens the Options window, which you can close at this stage, and then the Dashboard window, which informs you about the missing license.

        .. figure:: images/ses_license.PNG
           :alt: SEGGER Embedded Studio Dashboard notification about missing license

           No commercial-use license detected SES prompt

    #. Click :guilabel:`Activate Your Free License`.
       A request form appears.

    #. Fill in your information and click :guilabel:`Request License`.
       The license is sent to you in an email.

    #. After you receive your license key, click :guilabel:`Enter Activation Key` to activate the license.

    #. Copy-paste the license key and click :guilabel:`Install License`.
       The license activation window will close and SES will open the Project Explorer window.

.. note::
    Notifications about newer versions of SES are disabled in the SES Nordic Edition.
    This is because of the custom |NCS| options that are available only in the Nordic Edition.
    You will not be able to use SES with the |NCS| if you install the latest standard version of SES.

    When you move to a newer release of the |NCS|, check the :ref:`gs_recommended_versions` page for the given release to see if you are using the minimum required version of SES Nordic Edition.
    If you need to install the latest version of the SES Nordic Edition, follow the steps described above and make sure to set up the build environment in SES once more.

.. _build_environment:
.. _setting_up_SES:

Set up the build environment in SES
***********************************

If you chose to use |SES| for :ref:`building and programming a sample application <gs_programming>`, you must first set up your build environment.

1. Set up the SES environment.
   If you plan to :ref:`build with SEGGER Embedded Studio <gs_programming_ses>`, the first time you import an |NCS| project, SES might prompt you to set the paths to the Zephyr Base directory and the GNU ARM Embedded Toolchain.
   You only need to do this once.
   Complete the following steps:

   a. Navigate to the :file:`bin` directory.
   #. Run the :file:`emStudio` executable file.
   #. Select :guilabel:`File` > :guilabel:`Open nRF Connect SDK Project`.

      .. figure:: images/ses_open.png
         :alt: Open nRF Connect SDK Project menu

         Open nRF Connect SDK Project menu

   #. Set the Zephyr Base directory to the full path to :file:`ncs/zephyr`.
      The GNU ARM Embedded Toolchain directory is the directory where you installed the toolchain (for example, :file:`c:/gnuarmemb`).

      .. figure:: images/ses_notset.png
         :alt: Zephyr Base Not Set prompt

         Zephyr Base Not Set prompt

#. Set up executables.
   The process is different depending on your operating system.

   .. tabs::

      .. group-tab:: Windows

         Make sure the locations of executable tools are added to the :envvar:`PATH` variable, including Python, CMake, Ninja, and DTC.
         On Windows, SES uses the :envvar:`PATH` variable to find executables if they are not set in SES.

      .. group-tab:: Linux

         Make sure the locations of executable tools are added to the :envvar:`PATH` variable, including Python, CMake, Ninja, and DTC.
         On Linux, SES uses the :envvar:`PATH` variable to find executables if they are not set in SES.

      .. group-tab:: macOS

         If you start SES on macOS by running the :file:`emStudio` executable file, make sure to complete the following steps:

         1. Specify the path to all executables under :guilabel:`Tools` > :guilabel:`Options` (in the :guilabel:`nRF Connect` tab).

            .. figure:: images/ses_options.png
               :alt: nRF Connect SDK options in SES on Windows

               nRF Connect SDK options in SES (Windows)

            Use this section to change the SES environment settings later as well.

         #. Specify the path to the west tool as an additional CMake option, replacing *path_to_west* with the path to the west executable (for example, :file:`/usr/local/bin/west`):

            .. parsed-literal::
               :class: highlight

               -DWEST=\ *path_to_west*

         If you start SES from the command line, it uses the global :envvar:`PATH` variable to find the executables.
         You do not need to explicitly configure the executables in SES.

         Regardless of how you start SES, if you get an error that a tool or command cannot be found, first make sure that the tool is installed.
         If it is installed, verify that its path is configured correctly in the SES settings or in the :envvar:`PATH` variable.

   ..

If you want to change the SES environment settings after the initial setup, click :guilabel:`Tools` > :guilabel:`Options` and select the :guilabel:`nRF Connect` tab, as shown on the following screenshot from the Windows installation.

.. _ses_options_figure:

   .. figure:: images/ses_options.png
      :alt: nRF Connect SDK options in SES on Windows

      nRF Connect SDK options in SES (Windows)

If you want to configure tools that are not listed in the SES options, add them to the :envvar:`PATH` variable.

.. _build_environment_cli:

Set up the command-line build environment
*****************************************

In addition to |VSC| and |SES|, you can also build and program your application from the command line.
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
