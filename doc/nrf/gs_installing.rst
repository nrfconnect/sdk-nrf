.. _gs_installing:

Installing the |NCS| manually
#############################

The recommended way to get started with the |NCS| is to run the Getting Started Assistant app in nRF Connect for Desktop.
See the :ref:`gs_assistant` section for information about how to install the Getting Started Assistant.

.. note::
   If you use the Getting Started Assistant, you can skip this section of the documentation.
   If you prefer to install the toolchain manually, or if you run into problems during the installation process, see the following documentation for instructions.

To manually install the |NCS|, you must install all required tools and clone the |NCS| repositories.
See the following sections for detailed instructions.

The first two steps, `Installing the required tools`_ and `Installing the toolchain`_, are identical to the installation in Zephyr.
If you already have your system set up to work with the Zephyr OS, you can skip these steps.

See :ref:`gs_installing_os` for information on the supported operating systems and Zephyr features.

.. _gs_installing_tools:

Installing the required tools
*****************************

The installation process is different depending on your operating system.

.. tabs::

   .. group-tab:: Windows

      The recommended way for installing the required tools on Windows is to use Chocolatey, a package manager for Windows.
      Chocolatey installs the tools so that you can use them from a Windows command prompt.

      To install the required tools, follow Zephyr's Getting Started Guide.
      Note that there are several ways to install the tools.
      We recommend installing according to :ref:`zephyr:windows_install_native`.

   .. group-tab:: Linux

      To install the required tools on Linux, follow the :ref:`zephyr:linux_requirements` section of Zephyr's Getting Started Guide.

      In addition, make sure that you have dtc v1.4.6 or later installed.
      Depending on the Linux distribution that you use, you might need to install it manually because the current official package version might be older than v1.4.6.
      If you use Ubuntu, install v1.4.7 from Cosmic by entering the following commands::

         wget http://mirrors.kernel.org/ubuntu/pool/main/d/device-tree-compiler/device-tree-compiler_1.4.7-1_amd64.deb
         sudo dpkg -i device-tree-compiler_1.4.7-1_amd64.deb

      .. note::
         You do not need to install the Zephyr SDK.
         We recommend to install the compiler toolchain separately, as detailed in `Installing the toolchain`_.

   .. group-tab:: macOS

      To install the required tools, follow the :ref:`zephyr:mac_requirements` section of Zephyr's Getting Started Guide.

      Install Homebrew and install the required tools using the ``brew`` command line tool.

.. _gs_installing_toolchain:

Installing the toolchain
************************

To be able to cross-compile your applications for Arm targets, you must install  version 8-2019-q3-update of the `GNU Arm Embedded Toolchain`_.

.. important::
   Make sure to install the version that is mentioned above.
   Other versions might not work with this version of the nRF Connect SDK.

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

          Add the environment variables as system environment variables or define them in the ``%userprofile%\zephyrrc.cmd`` file as described in `Setting up the build environment`_.
          This will allow you to avoid setting them every time you open a command prompt.

       .. group-tab:: Linux

          Define the environment variables in the ``~/.zephyrrc`` file as described in `Setting up the build environment`_.
          This will allow you to avoid setting them every time you open a terminal window.

       .. group-tab:: macOS

          Define the environment variables in the ``~/.zephyrrc`` file as described in `Setting up the build environment`_.
          This will allow you to avoid setting them every time you open a terminal window.


.. _cloning_the_repositories_win:
.. _cloning_the_repositories:

Getting the |NCS| code
**********************

The |NCS| consists of a set of Git repositories.

Every |NCS| release consists of a combination of these repositories at different revisions.
The revision of each of those repositories is determined by the current revision of the main (or manifest) repository, `fw-nrfconnect-nrf`_.

.. note::
   The latest state of development is on the master branch of the `fw-nrfconnect-nrf`_ repository.
   To ensure a usable state, the `fw-nrfconnect-nrf`_ repository defines the compatible states of the other repositories.
   However, this state is not necessarily tested.
   For a higher degree of quality assurance, check out a tagged release.

   Therefore, unless you are familiar with the development process, you should always work with a specific release of the |NCS|.

To manage the combination of repositories and versions, the |NCS| uses :ref:`zephyr:west`.
The main repository, `fw-nrfconnect-nrf`_, contains a `west manifest file`_, :file:`west.yml`, that determines the revision of all other repositories.
This means that fw-nrfconnect-nrf acts as the :ref:`manifest repository <zephyr:west-multi-repo>`, while the other repositories are project repositories.

You can find additional information about the repository and development model in the :ref:`development model section <dev-model>`.

See the :ref:`west documentation <zephyr:west>` for detailed information about the tool itself.

Installing west
===============

Install the bootstrapper for west by entering the following command:

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
Like any other Python package, the west bootstrapper is updated regularly.
Therefore, remember to regularly check for updates:

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


Cloning the repositories
========================

.. tip::
   If you already cloned the |NCS| repositories in Git and want to continue using these clones instead of creating new ones, see `Updating your existing clones to use west`_.

To clone the repositories, complete the following steps:

1. Create a folder named ``ncs``.
   This folder will hold all |NCS| repositories.
#. Open a command window in the ``ncs`` folder.
#. Initialize west with the revision of the |NCS| that you want to check out:

   * To check out a specific release, go to the :ref:`release_notes` of that release and find the corresponding tag.
     Then enter the following command, replacing *NCS_version* with the tag:

     .. parsed-literal::
        :class: highlight

        west init -m https\://github.com/NordicPlayground/fw-nrfconnect-nrf --mr *NCS_version*

     .. note::
        * West was introduced after |NCS| v0.3.0.
          Therefore, you cannot use it to check out v0.1.0 or v0.3.0.

        * Initializing west with a specific revision of the manifest file does not lock your repositories to this version.
          Checking out a different branch or tag in the repositories changes the version of the |NCS| that you work with.
   * To check out the latest state of development, enter the following command:

     .. parsed-literal::
        :class: highlight

        west init -m https\://github.com/NordicPlayground/fw-nrfconnect-nrf

   * More generally, to check out an arbitrary revision, enter the following command, replacing *NCS_revision* with the revision that you want to check out:

     .. parsed-literal::
        :class: highlight

        west init -m https\://github.com/NordicPlayground/fw-nrfconnect-nrf --mr *NCS_revision*

     .. note::
        *NCS_revision* can be a branch (for example, ``master``), a tag (for example, ``v1.2.0``), or even a SHA (for example, ``224bee9055d986fe2677149b8cbda0ff10650a6e``). When not specified, it defaults to ``master``.

   This will clone the manifest repository `fw-nrfconnect-nrf`_ into :file:`nrf`.

#. Enter the following command to clone the project repositories::

      west update

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

Updating the repositories
=========================

If you work with a specific release of the |NCS|, you do not need to update your repositories, because the release will not change.
However, you might want to switch to a newer release or check out the latest state of development.

To manage the ``nrf`` repository (the manifest repository), use Git.
To make sure that you have the latest changes, run ``git fetch origin`` to :ref:`fetch the latest code <dm-wf-update-ncs>` from the `fw-nrfconnect-nrf`_ repository.
Checking out a branch or tag in the ``nrf`` repository gives you a different version of the manifest file.
Running ``west update`` will then update the project repositories to the state specified in this manifest file.
For example, to switch to release v1.2.0 of the |NCS|, enter the following commands in the ``ncs/nrf`` directory::

   git fetch origin
   git checkout v1.2.0
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

Updating your existing clones to use west
=========================================

If you already cloned the |NCS| repositories in Git and want to continue using these clones instead of creating new ones, you can initialize west to use your clones.
All branches, remotes, and other configuration in your repositories will be maintained.

To update your repositories to be managed by west, make sure that they are structured and named in the following way::

   ncs
    |___ bootloader/mcuboot
    |___ nrf
    |___ nrfxlib
    |___ zephyr
    |___ ...

Then complete the following steps:

1. In the ``ncs`` folder, open a command prompt or terminal window (depending on your operating system).
#. Do a ``git pull`` or rebase your branch so that you are on the latest fw-nrfconnect-nrf master.
#. Navigate one folder level up to the ``ncs`` folder::

      cd ..

#. Initialize west with the manifest folder from the current branch of your ``nrf`` repository::

      west init -l nrf

   This will create the required ``.west`` folder that is linked to the manifest repository (``nrf``).
#. Enter the following command to clone or update the project repositories::

      west update


.. _additional_deps:

Installing additional Python dependencies
*****************************************

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





.. _installing_ses:

Installing |SES| Nordic Edition
*******************************

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

Setting up the build environment
********************************

Before you start :ref:`building and programming a sample application <gs_programming>`, you must set up your build environment.

Setting up the SES environment
==============================

If you plan to :ref:`build with SEGGER Embedded Studio <gs_programming_ses>`, the first time you import an |NCS| project, SES will prompt you to set the paths to the Zephyr Base directory and the GNU ARM Embedded Toolchain.
This must be done only once.

Complete the following steps to set up the |SES| environment:

1. Run the file :file:`bin/emStudio`.

#. Select :guilabel:`File` -> :guilabel:`Open nRF Connect SDK Project`.

    .. figure:: images/ses_open.png
       :alt: Open nRF Connect SDK Project menu

       Open nRF Connect SDK Project menu

#. Set the Zephyr Base directory to the full path to ``ncs/zephyr``.
   The GNU ARM Embedded Toolchain directory is the directory where you installed the toolchain (for example, ``c:/gnuarmemb``).

    .. figure:: images/ses_notset.png
       :alt: Zephyr Base Not Set prompt

       Zephyr Base Not Set prompt

Setting up executables
======================

The process is different depending on your operating system.

.. tabs::

   .. group-tab:: Windows

      Make sure the locations of tools are added to the PATH variable.
      On Windows, SES uses the PATH variable to find executables if they are not set in SES.

   .. group-tab:: Linux

      Make sure the locations of tools are added to the PATH variable.
      On Linux, SES uses the PATH variable to find executables if they are not set in SES.

   .. group-tab:: macOS

      If you start SES on macOS by running the file :file:`bin/emStudio`, make sure to complete the following steps:

      1. Specify the path to all executables under :guilabel:`Tools` -> :guilabel:`Options` (in the :guilabel:`nRF Connect` tab).

          .. figure:: images/ses_options.png
               :alt: nRF Connect SDK options in SES on Windows

               nRF Connect SDK options in SES (Windows)

         Use this section to change the SES environment settings later as well.

      #. Specify the path to the west tool as additional CMake option, replacing *path_to_west* with the path to the west executable (for example, ``/usr/local/bin/west``):

          .. parsed-literal::
             :class: highlight

             -DWEST=\ *path_to_west*


      If you start SES from the command line, it uses the global PATH variable to find the executables.
      You do not need to explicitly configure the executables in SES.

      Regardless of how you start SES, if you get an error that a tool or command cannot be found, first make sure that the tool is installed.
      If it is installed, verify that its path is configured correctly in the SES settings or in the PATH variable.


Changing the SES environment settings
=====================================

If you want to change the SES environment settings, click :guilabel:`Tools` -> :guilabel:`Options` and select the :guilabel:`nRF Connect` tab, as shown on the following screenshot from the Windows installation.

.. _ses_options_figure:

.. figure:: images/ses_options.png
     :alt: nRF Connect SDK options in SES on Windows

     nRF Connect SDK options in SES (Windows)

If you want to configure tools that are not listed in the SES options, add them to the PATH variable.

.. _build_environment_cli:

Setting up the command line build environment
=============================================

If you want to build and program your application from the command line, you must set up your build environment by defining the required environment variables every time you open a new command prompt or terminal window.

See :ref:`zephyr:env_vars_important` information about the various relevant environment variables.

Define the required environment variables as follows, depending on your operating system:

.. tabs::

   .. group-tab:: Windows

      Navigate to the ``ncs`` folder and enter the following command: ``zephyr\zephyr-env.cmd``

      If you need to define additional environment variables, create the file ``%userprofile%\zephyrrc.cmd`` and add the variables there.
      This file is loaded automatically when you run the above command.

   .. group-tab:: Linux

      Navigate to the ``ncs`` folder and enter the following command: ``source zephyr/zephyr-env.sh``

      If you need to define additional environment variables, create the file ``~/.zephyrrc`` and add the variables there.
      This file is loaded automatically when you run the above command.

   .. group-tab:: macOS

      Navigate to the ``ncs`` folder and enter the following command: ``source zephyr/zephyr-env.sh``

      If you need to define additional environment variables, create the file ``~/.zephyrrc`` and add the variables there.
      This file is loaded automatically when you run the above command.

You must also make sure that nrfjprog (part of the `nRF Command Line Tools`_) is installed and its path is added to the environment variables.
The west command programs the board by using nrfjprog by default.
For more information on nrfjprog, see `programming development boards using nrfjprog <Programming DK boards using nrfjprog_>`_.
