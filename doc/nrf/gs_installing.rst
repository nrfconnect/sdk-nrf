.. _gs_installing:

Installing the |NCS| manually
#############################

.. contents::
   :local:
   :depth: 2

The recommended way to get started with the |NCS| is to use nRF Connect for Desktop.
See the :ref:`gs_assistant` section for information about how to install the |NCS| through nRF Connect for Desktop.

.. note::
   If you use nRF Connect for Desktop to install the |NCS|, you can skip this section of the documentation.
   If you prefer to install the toolchain manually, or if you run into problems during the installation process, see the following documentation for instructions.

To manually install the |NCS|, you must install all required tools and clone the |NCS| repositories.
See the following sections for detailed instructions.

The steps `Install the required tools`_ and `Install the toolchain`_ are similar to the installation steps in Zephyr's :ref:`zephyr:getting_started`.
If you already have your system set up to work with the Zephyr OS, you can skip these steps.

See :ref:`gs_installing_os` for information on the supported operating systems and Zephyr features.

.. _gs_installing_tools:

.. rst-class:: numbered-step

Install the required tools
**************************

The installation process is different depending on your operating system.

.. tabs::

   .. group-tab:: Windows

      The recommended way for installing the required tools on Windows is to use Chocolatey, a package manager for Windows.
      Chocolatey installs the tools so that you can use them from a Windows command prompt.

      To install the required tools, follow the :ref:`install-required-tools` section for Windows in Zephyr's :ref:`zephyr:getting_started`.

   .. group-tab:: Linux

      To install the required tools on Linux, follow the :ref:`install-required-tools` section for Linux in Zephyr's :ref:`zephyr:getting_started`.
      Additional information is available in the :ref:`zephyr:linux_requirements` section.

      .. note::
         You do not need to install the Zephyr SDK.
         We recommend to install the compiler toolchain separately, as detailed in `Install the toolchain`_.

   .. group-tab:: macOS

      To install the required tools, follow the :ref:`install-required-tools` section for macOS in Zephyr's :ref:`zephyr:getting_started`.

      Install Homebrew and install the required tools using the ``brew`` command line tool.

      Also see :ref:`zephyr:mac-setup-alts` for additional information.

..

In addition to these required tools, install the `GN`_ meta-build system if you are interested in building `Project Connected Home over IP`_ applications.
This system generates the Ninja files that the |NCS| uses.

.. tabs::

   .. group-tab:: Windows

      To install the GN tool, complete the following steps:

      1. Download the latest version of the GN binary archive for Windows from the `GN website`_.
      2. Extract the :file:`zip` archive.
      3. Ensure that the GN tool is added to your :envvar:`PATH` :ref:`environment variable <zephyr:env_vars>`.

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
      #. Add the location of the GN tool to the system PATH.
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

      #. Add the location of the GN tool to the system PATH.
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

To install west, enter the following command:

.. tabs::

   .. group-tab:: Windows

      .. parsed-literal::
         :class: highlight

         pip3 install west

   .. group-tab:: Linux

      .. parsed-literal::
         :class: highlight

         pip3 install --user west

   .. group-tab:: macOS

      .. parsed-literal::
         :class: highlight

         pip3 install west

You only need to do this once.

.. _cloning_the_repositories_win:
.. _cloning_the_repositories:

.. rst-class:: numbered-step

Get the |NCS| code
******************

The |NCS| consists of a set of :ref:`Git repositories <ncs_introduction>`.

Every |NCS| release consists of a combination of these repositories at different revisions.
The revision of each of those repositories is determined by the current revision of the main (or manifest) repository, `sdk-nrf`_.

.. note::
   The latest state of development is on the master branch of the `sdk-nrf`_ repository.
   To ensure a usable state, the `sdk-nrf`_ repository defines the compatible states of the other repositories.
   However, this state is not necessarily tested.
   For a higher degree of quality assurance, check out a tagged release.

   Therefore, unless you are familiar with the development process, you should always work with a specific release of the |NCS|.

The main repository, `sdk-nrf`_, contains a `west manifest file`_, :file:`west.yml`, that determines the revision of all other repositories.
This means that sdk-nrf acts as the :ref:`manifest repository <zephyr:west-basics>`, while the other repositories are project repositories.

For more information about the repository and development model, see the :ref:`development model section <dev-model>`.

.. tip::
   If you cloned the |NCS| repositories before they were moved to the nrfconnect GitHub organization and want to update them, follow the instructions in :ref:`repo_move`.

To clone the repositories, complete the following steps:

1. Create a folder named ``ncs``.
   This folder will hold all |NCS| repositories.
#. Open a command window in the ``ncs`` folder.
#. Determine what revision of the |NCS| you want to work with.
   The recommended way is to work with a specific release.

   * To work with a specific release, the revision is the corresponding tag (for example, |release_tt|).
     You can find the tag in the :ref:`release_notes` of the release.
   * To work with a development tag, the revision is the corresponding tag (for example, ``v1.2.99-dev1``)
   * To work with a branch, the revision is the branch name (for example, ``master`` to work with the latest state of development).
   * To work with a specific state, the revision is the SHA (for example, ``224bee9055d986fe2677149b8cbda0ff10650a6e``).

#. Initialize west with the revision of the |NCS| that you want to check out, replacing *NCS_revision* with the revision:

   .. parsed-literal::
      :class: highlight

      west init -m https\://github.com/nrfconnect/sdk-nrf --mr *NCS_revision*

   For example, to check out the |release| release, enter the following command:

   .. parsed-literal::
      :class: highlight

      west init -m https\://github.com/nrfconnect/sdk-nrf --mr |release|

   To check out the latest state of development, enter the following command::

     west init -m https://github.com/nrfconnect/sdk-nrf --mr master

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

      Open a command prompt in the ``ncs`` folder and enter the following commands:

        .. parsed-literal::
           :class: highlight

           pip3 install -r zephyr/scripts/requirements.txt
           pip3 install -r nrf/scripts/requirements.txt
           pip3 install -r bootloader/mcuboot/scripts/requirements.txt

   .. group-tab:: Linux

      Open a terminal window in the ``ncs`` folder and enter the following commands:

        .. parsed-literal::
           :class: highlight

           pip3 install --user -r zephyr/scripts/requirements.txt
           pip3 install --user -r nrf/scripts/requirements.txt
           pip3 install --user -r bootloader/mcuboot/scripts/requirements.txt

   .. group-tab:: macOS

      Open a terminal window in the ``ncs`` folder and enter the following commands:

        .. parsed-literal::
           :class: highlight

           pip3 install -r zephyr/scripts/requirements.txt
           pip3 install -r nrf/scripts/requirements.txt
           pip3 install -r bootloader/mcuboot/scripts/requirements.txt

..

.. _gs_installing_toolchain:

.. rst-class:: numbered-step

Install the toolchain
*********************

To be able to cross-compile your applications for Arm targets, you must install version 9-2019-q4-major of the `GNU Arm Embedded Toolchain`_.

.. important::
   Make sure to install the version that is mentioned above.
   Other versions might not work with this version of the |NCS|.

   Other versions of the |NCS| might require a different toolchain version.

To set up the toolchain, complete the following steps:

.. _toolchain_setup:

1. Download the `GNU Arm Embedded Toolchain`_ for your operating system.
#. Extract the toolchain into a folder of your choice.
   We recommend to use the folder ``c:\gnuarmemb`` on Windows and ``~/gnuarmemb`` on Linux or macOS.

   Make sure that the folder name does not contain any spaces or special characters.
#. If you want to build and program applications from the command line, define the environment variables for the GNU Arm Embedded toolchain.
   Depending on your operating system:

    .. tabs::

       .. group-tab:: Windows

          Open a command prompt and enter the following commands (assuming that you have installed the toolchain to ``c:\gnuarmemb``; if not, change the value for GNUARMEMB_TOOLCHAIN_PATH):

            .. parsed-literal::
               :class: highlight

               set ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
               set GNUARMEMB_TOOLCHAIN_PATH=\ c:\\gnuarmemb

       .. group-tab:: Linux

          Open a terminal window and enter the following commands (assuming that you have installed the toolchain to ``~/gnuarmemb``; if not, change the value for GNUARMEMB_TOOLCHAIN_PATH):

            .. parsed-literal::
              :class: highlight

              export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
              export GNUARMEMB_TOOLCHAIN_PATH=\ "~/gnuarmemb"

       .. group-tab:: macOS

          Open a terminal window and enter the following commands (assuming that you have installed the toolchain to ``~/gnuarmemb``; if not, change the value for GNUARMEMB_TOOLCHAIN_PATH):

            .. parsed-literal::
              :class: highlight

              export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
              export GNUARMEMB_TOOLCHAIN_PATH=\ "~/gnuarmemb"

#. Set the environment variables persistently.
   Depending on your operating system:

    .. tabs::

       .. group-tab:: Windows

          Add the environment variables as system environment variables or define them in the ``%userprofile%\zephyrrc.cmd`` file as described in :ref:`build_environment_cli`.
          This lets you avoid setting them every time you open a command prompt.

       .. group-tab:: Linux

          Define the environment variables in the ``~/.zephyrrc`` file as described in :ref:`build_environment_cli`.
          This lets you avoid setting them every time you open a terminal window.

       .. group-tab:: macOS

          Define the environment variables in the ``~/.zephyrrc`` file as described in :ref:`build_environment_cli`.
          This lets you avoid setting them every time you open a terminal window.


.. _installing_ses:

.. rst-class:: numbered-step

Install |SES| Nordic Edition
****************************

You must install |SES| (SES) Nordic Edition to be able to open and compile projects in the |NCS|.

|SES| is free of charge for use with Nordic Semiconductor devices.

To install |SES| Nordic Edition, complete the following steps:

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

    a. Run the file :file:`bin/emStudio`.
       |SES| will open the Dashboard window and inform you about the missing license.

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

.. _build_environment:
.. _setting_up_SES:

.. rst-class:: numbered-step

Set up the build environment in SES
***********************************

Before you start :ref:`building and programming a sample application <gs_programming>`, you must set up your build environment.

1. Set up the SES environment.
   If you plan to :ref:`build with SEGGER Embedded Studio <gs_programming_ses>`, the first time you import an |NCS| project, SES might prompt you to set the paths to the Zephyr Base directory and the GNU ARM Embedded Toolchain.
   You only need to do this once.
   Complete the following steps:

   a. Run the file :file:`bin/emStudio`.

   #. Select :guilabel:`File` -> :guilabel:`Open nRF Connect SDK Project`.

      .. figure:: images/ses_open.png
         :alt: Open nRF Connect SDK Project menu

         Open nRF Connect SDK Project menu

   #. Set the Zephyr Base directory to the full path to ``ncs/zephyr``.
      The GNU ARM Embedded Toolchain directory is the directory where you installed the toolchain (for example, ``c:/gnuarmemb``).

      .. figure:: images/ses_notset.png
         :alt: Zephyr Base Not Set prompt

         Zephyr Base Not Set prompt

#. Set up executables.
   The process is different depending on your operating system.

   .. tabs::

      .. group-tab:: Windows

         Make sure the locations of executable tools are added to the PATH variable, including Python, CMake, Ninja, and DTC.
         On Windows, SES uses the PATH variable to find executables if they are not set in SES.

      .. group-tab:: Linux

         Make sure the locations of executable tools are added to the PATH variable, including Python, CMake, Ninja, and DTC.
         On Linux, SES uses the PATH variable to find executables if they are not set in SES.

      .. group-tab:: macOS

         If you start SES on macOS by running the file :file:`bin/emStudio`, make sure to complete the following steps:

         1. Specify the path to all executables under :guilabel:`Tools` -> :guilabel:`Options` (in the :guilabel:`nRF Connect` tab).

            .. figure:: images/ses_options.png
               :alt: nRF Connect SDK options in SES on Windows

               nRF Connect SDK options in SES (Windows)

            Use this section to change the SES environment settings later as well.

         #. Specify the path to the west tool as an additional CMake option, replacing *path_to_west* with the path to the west executable (for example, ``/usr/local/bin/west``):

            .. parsed-literal::
               :class: highlight

               -DWEST=\ *path_to_west*


         If you start SES from the command line, it uses the global PATH variable to find the executables.
         You do not need to explicitly configure the executables in SES.

         Regardless of how you start SES, if you get an error that a tool or command cannot be found, first make sure that the tool is installed.
         If it is installed, verify that its path is configured correctly in the SES settings or in the PATH variable.

   ..

#. Change the SES environment settings.
   If you want to change the SES environment settings, click :guilabel:`Tools` -> :guilabel:`Options` and select the :guilabel:`nRF Connect` tab, as shown on the following screenshot from the Windows installation.

.. _ses_options_figure:

   .. figure:: images/ses_options.png
      :alt: nRF Connect SDK options in SES on Windows

      nRF Connect SDK options in SES (Windows)

   If you want to configure tools that are not listed in the SES options, add them to the PATH variable.

.. _build_environment_cli:

Set up the command-line build environment
*****************************************

The default build environment for |NCS| is SES.
However, you can also build and program your application from the command line.
You have to set up your build environment by defining the required environment variables every time you open a new command prompt or terminal window.

See :ref:`zephyr:important-build-vars` for more information about the various relevant environment variables.

Define the required environment variables as follows, depending on your operating system:

.. tabs::

   .. group-tab:: Windows

      Navigate to the ``ncs`` folder and enter the following command: ``zephyr\zephyr-env.cmd``

      If you need to define additional environment variables, create the file ``%userprofile%\zephyrrc.cmd`` and add the variables there.
      This file is loaded automatically when you run the above command.
      See :ref:`zephyr:env_vars_zephyrrc` for more information.

   .. group-tab:: Linux

      Navigate to the ``ncs`` folder and enter the following command: ``source zephyr/zephyr-env.sh``

      If you need to define additional environment variables, create the file ``~/.zephyrrc`` and add the variables there.
      This file is loaded automatically when you run the above command.
      See :ref:`zephyr:env_vars_zephyrrc` for more information.


   .. group-tab:: macOS

      Navigate to the ``ncs`` folder and enter the following command: ``source zephyr/zephyr-env.sh``

      If you need to define additional environment variables, create the file ``~/.zephyrrc`` and add the variables there.
      This file is loaded automatically when you run the above command.
      See :ref:`zephyr:env_vars_zephyrrc` for more information.

You must also make sure that nrfjprog (part of the `nRF Command Line Tools`_) is installed and its path is added to the environment variables.
The west command programs the kit by using nrfjprog by default.
For more information on nrfjprog, see `Programming SoCs with nrfjprog`_.

.. _gs_updating:

Updating tools and repositories
*******************************

Remember to regularly check for updates to tools.
Like any other Python package, the west tool is updated regularly.

You might also want to switch to a newer release or check out the latest state of development.
However, if you work with a specific release of the |NCS|, you do not need to update your repositories, because the release will not change.

.. _west_update:

Updating west
=============

To update west, run the following command:

.. tabs::

   .. group-tab:: Windows

      .. parsed-literal::
         :class: highlight

         pip3 install -U west

   .. group-tab:: Linux

      .. parsed-literal::
         :class: highlight

         pip3 install --user -U west

   .. group-tab:: macOS

      .. parsed-literal::
         :class: highlight

         pip3 install -U west

..

This command updates west to the latest available version in the PyPi repository.

Updating the repositories
=========================

To manage the ``nrf`` repository (the manifest repository), use Git.
To make sure that you have the latest changes, run ``git fetch origin`` to :ref:`fetch the latest code <dm-wf-update-ncs>` from the `sdk-nrf`_ repository.
Checking out a branch or tag in the ``nrf`` repository gives you a different version of the manifest file.
Running ``west update`` updates the project repositories to the state specified in this manifest file.

.. include:: gs_installing.rst
   :start-after: west-error-start
   :end-before: west-error-end

Examples of commands
--------------------

To switch to release |release| of the |NCS|, enter the following commands in the ``ncs/nrf`` directory:

.. parsed-literal::
   :class: highlight

   git fetch origin
   git checkout |release|
   west update

To update to a particular revision (SHA), make sure that you have that particular revision locally before you check it out (by running ``git fetch origin``)::

   git fetch origin
   git checkout 224bee9055d986fe2677149b8cbda0ff10650a6e
   west update

To switch to the latest state of development, enter the following commands::

   git fetch origin
   git checkout origin/master
   west update

.. note::
   Run ``west update`` every time you change or modify the current working branch (for example, when you pull, rebase, or check out a different branch).
   This will bring the project repositories to the matching revision defined by the manifest file.

.. _repo_move:

Pointing the repositories to the right remotes after they were moved
====================================================================

Before |NCS| version 1.3.0, the Git repositories were moved from the NordicPlayground GitHub organization to the nrfconnect organization.
They were also renamed, replacing the ``fw-nrfconnect-`` prefix with ``sdk-``.

The full list of repositories with their old and new URLs is in the following table:

.. list-table::
   :header-rows: 1

   * - Old URL
     - New URL
     - File system path

   * - https:\ //github.com/NordicPlayground/fw-nrfconnect-nrf
     - https://github.com/nrfconnect/sdk-nrf
     - :file:`ncs/nrf`

   * - https:\ //github.com/NordicPlayground/fw-nrfconnect-zephyr
     - https://github.com/nrfconnect/sdk-zephyr
     - :file:`ncs/zephyr`

   * - https:\ //github.com/NordicPlayground/fw-nrfconnect-mcuboot
     - https://github.com/nrfconnect/sdk-mcuboot
     - :file:`ncs/bootloader/mcuboot`

   * - https:\ //github.com/NordicPlayground/fw-nrfconnect-mcumgr
     - https://github.com/nrfconnect/sdk-mcumgr
     - :file:`ncs/modules/lib/mcumgr`

   * - https:\ //github.com/NordicPlayground/nrfxlib
     - https://github.com/nrfconnect/sdk-nrfxlib
     - :file:`ncs/nrfxlib`


If you cloned the repositories before the move, your local repositories and forks of the |NCS| repositories are automatically be redirected to the new ones.
However, you should point them directly to their new locations as described in this section.

To change the remotes, complete the following steps:

1. Rename any personal forks that you have of the |NCS| repositories to their new names:

   a. Visit your personal fork in a browser.
      For example, to rename the fw-nrfconnect-nrf repository, access your fork on GitHub (``https://github.com/<username>/fw-nrfconnect-nrf``, where *<username>* is your GitHub account name).
   b. Switch to the :guilabel:`Settings` tab and edit the name in the :guilabel:`Repository name` field to ``sdk-nrf``.

#. Rename the remotes:

   a. Go to your local copy of each of the repositories listed in the preceding table.
   #. Enter the following command:

      .. parsed-literal::
        :class: highlight

        git remote set-url *remote_name* *new_url*

      In this command, replace *remote_name* with the name of your remote (for example, ``origin`` for your fork or ``ncs`` for the upstream repository) and *new_url* with the URL of your fork or the new URL from the preceding table.
      If you are not sure about the remotes that are configured in your local repository, enter ``git remote -v`` to see your remotes.

For example, to point your existing fw-nrfconnect-nrf clone to its new URL, enter the following command:

.. code-block::

   cd ncs/nrf
   git remote set-url origin https://github.com/nrfconnect/sdk-nrf

Similarly, to point your existing fw-nrfconnect-zephyr clone to the new URL, enter the following command:

.. code-block::

   cd  ncs/zephyr
   git remote set-url ncs https://github.com/nrfconnect/sdk-zephyr

For more information about remotes and how to handle them, see :ref:`dm-wf-fork`.
