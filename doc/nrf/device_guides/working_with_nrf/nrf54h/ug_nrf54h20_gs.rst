:orphan:

.. _ug_nrf54h20_gs:

Getting started with the nRF54H20 PDK
#####################################

.. contents::
   :local:
   :depth: 2

This page will get you started with your nRF54H20 PDK using the |NCS|.
It first tells you how to test that your PDK is working correctly by using the preflashed :zephyr:code-sample:`blinky` sample.
After the test, it will guide you on how to configure, build, and program the :ref:`multicore_hello_world` sample to the development kit and read logs from it.

If you have already set up your nRF54H20 PDK and want to learn more, see the :ref:`ug_nrf54h20_architecture` documentation for an in-depth description of the architecture of the nRF54H20.

.. _ug_nrf54h20_gs_requirements:

Minimum requirements
********************

Make sure you have all the required hardware and that your computer has one of the supported operating systems.

Hardware
========

* nRF54H20 PDK
* USB-C cable

Software
========

On your computer, one of the following operating systems:

* Microsoft Windows
* Ubuntu Linux
* macOS

|supported OS|

You also need to install `Git`_ or `Git for Windows`_ (on Linux and Mac, or Windows, respectively).

.. _ug_nrf54h20_gs_test_blinky:

Testing with the Blinky sample
******************************

In the limited sampling release, the nRF54H20 PDK comes preprogrammed with the :zephyr:code-sample:`blinky` sample.

Complete the following steps to test that the PDK works correctly:

1. Connect the USB-C end of the USB-C cable to the **IMCU USB** port the nRF54H20 PDK.
#. Connect the other end of the USB-C cable to your PC.
#. Move the **POWER** switch to **On** to turn the nRF54H20 PDK on.
   The switch is located in the top left corner of the PDK PCB.

**LED1** will turn on and start to blink.

If something does not work as expected, contact Nordic Semiconductor support.

.. _nrf54h20_gs_installing_software:

Installing the required software
********************************

To work with the nRF54H20 PDK, you need to install the limited sampling version of the |NCS|.
Follow the instructions in the next sections to install the required tools.

.. _nrf54h20_install_commandline:

Installing the nRF Command Line Tools
=====================================

You need the nRF Command Line Tools specific to the limited sampling release of the |NCS|.

To install the nRF Command Line Tools, you need to download and install the version corresponding to your system:

* `10.23.3_ec 64-bit Windows, executable`_
* `10.23.3_ec macOS, zip archive`_
* 64-bit Linux:

  * `10.23.3_ec x86 system, deb format`_
  * `10.23.3_ec x86 system, RPM`_
  * `10.23.3_ec x86 system, tar archive`_

  * `10.23.3_ec ARM64 system, deb format`_
  * `10.23.3_ec ARM64 system, RPM`_
  * `10.23.3_ec ARM64 system, tar archive`_

* 32-bit Linux:

  * `10.23.3_ec ARMHF system, zip archive`_

.. _nrf54h20_install_toolchain:

Installing the toolchain
========================

You can install the toolchain for the limited sampling of the |NCS| by running an installation script.
Before installing it, however, you need to have been granted an access to the necessary GitHub repositories using an authenticated account that does not have a passphrase key for credentials.
The access is granted as part of the onboarding process for the limited sampling release.
Ensure that you additionally have Git and curl installed.

.. tabs::

   .. tab:: Windows

      Follow these steps:

      1. Create on GitHub your `Personal Access Token (PAT)`_.
      #. Open git bash.
      #. Download and run the :file:`bootstrap-toolchain.sh` installation script file using the following command:

         .. parsed-literal::
            :class: highlight

            curl --proto '=https' --tlsv1.2 -sSf https://developer.nordicsemi.com/.pc-tools/scripts/bootstrap-toolchain.sh | NCS_TOOLCHAIN_VERSION=v2.4.99-cs3 sh

         Depending on your connection, this might take some time.
         Use your GitHub username and Personal Access Token (PAT) when prompted to.
      #. Run the following command in Git Bash:

         .. parsed-literal::
            :class: highlight

            c:/ncs-lcs/nrfutil.exe toolchain-manager launch --terminal --chdir "c:/ncs-lcs/work-dir" --ncs-version v2.4.99-cs3

         This opens a new terminal window with the |NCS| toolchain environment, where west and other development tools are available.
         Alternatively, you can run the following command::

            c:/ncs-lcs/nrfutil.exe toolchain-manager env --as-script

         This gives all the necessary environmental variables you need to copy-paste and execute in the same terminal window to be able to run west directly there.

         .. caution::
            When working with the limited sampling release, you must always use the terminal window where the west environmental variables have been called.

      #. Install the `Serial Terminal from nRF Connect for Desktop`_.

      If you run into errors during the installation process, delete the :file:`.west` folder inside the :file:`C:\\ncs-lcs` directory, and start over.

      We recommend adding the nrfutil path to your environmental variables.


   .. tab:: Linux

      Follow these steps:

      1. Create on GitHub your `Personal Access Token (PAT)`_.
      #. Open a terminal window.
      #. Download and run the :file:`bootstrap-toolchain.sh` installation script file using the following command:

         .. parsed-literal::
            :class: highlight

            curl --proto '=https' --tlsv1.2 -sSf https://developer.nordicsemi.com/.pc-tools/scripts/bootstrap-toolchain.sh | NCS_TOOLCHAIN_VERSION=v2.4.99-cs3 sh

         Depending on your connection, this might take some time.
         Use your GitHub username and Personal Access Token (PAT) when prompted to.
      #. Run the following command in your terminal:

         .. parsed-literal::
            :class: highlight

            $HOME/ncs-lcs/nrfutil toolchain-manager launch --shell --chdir "$HOME/ncs-lcs/work-dir" --ncs-version v2.4.99-cs3

         This makes west and other development tools in the |NCS| toolchain environment available in the same shell session.

         .. caution::
            When working with west in the limited sampling release version of |NCS|, you must always use this shell window.

      #. Install the `Serial Terminal from nRF Connect for Desktop`_.

      If you run into errors during the installation process, delete the :file:`.west` folder inside the :file:`ncs-lcs` directory, and start over.

      We recommend adding the nrfutil path to your environmental variables.

   .. tab:: macOS

      Follow these steps:

      1. Create on GitHub your `Personal Access Token (PAT)`_.
      #. Open a terminal window.
      #. Install `Homebrew`_:

         .. code-block:: bash

            /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

      #. Use ``brew`` to install the required dependencies:

         .. code-block:: bash

            brew install cmake ninja gperf python3 ccache qemu dtc wget libmagic

         Ensure that these dependencies are installed with their versions as specified in the :ref:`Required tools table <req_tools_table>`.
         To check the installed versions, run the following command:

         .. parsed-literal::
            :class: highlight

             brew list --versions

      #. Download and run the :file:`bootstrap-toolchain.sh` installation script file using the following command:

         .. parsed-literal::
            :class: highlight

            curl --proto '=https' --tlsv1.2 -sSf https://developer.nordicsemi.com/.pc-tools/scripts/bootstrap-toolchain.sh | NCS_TOOLCHAIN_VERSION=v2.4.99-cs3 sh

         Depending on your connection, this might take some time.
         Use your GitHub username and Personal Access Token (PAT) when prompted to.

         .. note::
            On macOS, the install directory is :file:`/opt/nordic/ncs`.
            This means that creating the directory requires root access.
            You will be prompted to grant the script admin rights for the creation of the folder on the first install.
            The folder will be created with the necessary access rights to the user, so subsequent installs do not require root access.

            Do not run the toolchain-manager installation as root (for example, using `sudo`), as this would cause the directory to only grant access to root, meaning subsequent installations will also require root access.
            If you run the script as root, to fix permissions delete the installation folder and run the script again as a non-root user.

      #. Run the following command in your terminal:

         .. parsed-literal::
            :class: highlight

            /Users/*yourusername*/ncs-lcs/nrfutil toolchain-manager launch --shell --chdir "/Users/*yourusername*/ncs-lcs/work-dir" --ncs-version v2.4.99-cs3

         This makes west and other development tools in the |NCS| toolchain environment available in the same shell session.

         .. caution::
            When working with west in the limited sampling release version of |NCS|, you must always use this shell window.

      #. Run the following commands in your terminal to install the correct lxml dependency:

         .. parsed-literal::
            :class: highlight

            pip uninstall -y lxml
            pip install lxml

      #. Install the `Serial Terminal from nRF Connect for Desktop`_.

      If you run into errors during the installation process, delete the :file:`.west` folder inside the :file:`ncs-lcs` directory, and start over.

      We recommend adding the nrfutil path to your environmental variables.

.. _nrf54h20_install_ncs:

Installing the |NCS|
====================

After you have installed nRF Command Line Tools and the toolchain, complete the following steps to get the limited sampling version of the |NCS|:

1. In the terminal window opened during the toolchain installation, initialize west with the revision of the |NCS| from the limited sampling by running the following command:

   .. parsed-literal::
      :class: highlight

      west init -m https://github.com/nrfconnect/sdk-nrf-next --mr v2.4.99-cs3

   A window pops up to ask you to select a credential helper.
   You can use any of the options.

#. Set up GitHub authentication:

   ``west update`` requires :ref:`west <zephyr:west>` to fetch from private repositories on GitHub.

   There are two ways you can authenticate when accessing private repositories on GitHub:

   * Using SSH authentication, where your git remotes URLs use ``ssh://``.
   * Using HTTPS authentication, where your git remotes URLs use ``https://``.

   GitHub has a comprehensive `documentation page on authentication methods`_.

   However, we suggest to choose your authentication method depending on your scenario:

   * If this is the first time you are setting up GitHub access, use HTTPS.
   * If you already have a git credentials file, use HTTPS.
   * If you already have an SSH key generated and uploaded to GitHub, use SSH.
   * If you are still undecided, use HTTPS.

   .. tabs::

      .. tab:: HTTPS authentication

          The `west manifest file`_ in the |NCS| uses ``https://`` URLs instead of ``ssh://``.
          When using HTTPS, you may be prompted to type your GitHub username and password or multiple times.
          This can be avoided by creating on GitHub a Personal Access Token (PAT) (needed for two-factor authentication) and using `Git Credential Manager`_ (included in the git installation) to store your credentials in git and handle GitHub authentication.

          1. Store your GitHub credentials (your username and the PAT created before) on disk using the ``store`` command from the git credential helper.

             .. code-block:: shell

                git config --global credential.helper store

          #. Create a :file:`~/.git-credentials` (or :file:`%userprofile%\\.git-credentials` on Windows) and add this line to it::

                https://<GitHub username>:<Personal Access Token>@github.com

             See the `git-credential-store`_ manual page for details.

          If you don't want to store any credentials on the file system, you can store them in memory temporarily using `git-credential-cache`_ instead.

      .. tab:: SSH authentication

          The `west manifest file`_ in the |NCS| uses ``https://`` URLs instead of ``ssh://``.
          If you are already using `SSH-based authentication`_, you can reuse your SSH setup by adding the following to your :file:`~/.gitconfig` (or :file:`%userprofile%\\.gitconfig` on Windows):

             .. parsed-literal::
                :class: highlight

                   [url "ssh://git@github.com"]
                         insteadOf = https://github.com

          This will rewrite the URLs on the fly so that Git uses ``ssh://`` for all network operations with GitHub.

          You achieve the same result also using Git Credential Manager:

          .. code-block:: shell

                git config --global credential.helper store
                git config --global url."git@github.com:".insteadOf "https://github.com/"

          If your SSH key has no password, fetching should just work. If it does have a
          password, you can avoid entering it manually every time using `ssh-agent`_.

          On GitHub, see `Connecting to GitHub with SSH`_ for details on configuration
          and key creation.

#. Enter the following command to clone the project repositories::

      west update

   Depending on your connection, this might take some time.

#. Export a :ref:`Zephyr CMake package <zephyr:cmake_pkg>`.
   This allows CMake to automatically load the boilerplate code required for building |NCS| applications::

      west zephyr-export

#. As an administrator (or superuser), install the requirements from `nrf-regtool`_ using the following command::

      pip install -r modules/lib/nrf-regtool/scripts/requirements-base.txt

Your directory structure now looks similar to this::

    ncs-lcs/work-dir
    |___ .west
    |___ bootloader
    |___ modules
    |___ nrf
    |___ nrfxlib
    |___ zephyr
    |___ ...


Note that there are additional folders, and that the structure might change.
The full set of repositories and folders is defined in the manifest file.

.. _ug_nrf54h20_gs_sample:

Programming the sample
**********************

The :ref:`multicore_hello_world` sample is a multicore sample running on both the Application core (``cpuapp``) and the Peripheral Processor (PPR, ``cpuppr``).
It uses the ``nrf54h20dk_nrf54h20_cpuapp@soc1`` build target.

To build and program the sample to the nRF54H20 PDK, complete the following steps:

1. Connect the nRF54H20 PDK to you computer using the IMCU USB port on the PDK.
#. Navigate to the :file:`nrf/samples/multicore/hello_world` folder containing the sample.
#. Build the sample by running the following command::

      west build -b nrf54h20dk_nrf54h20_cpuapp@soc1

#. Program the sample using nrfjprog.
   If you have multiple Nordic Semiconductor devices, make sure that only the nRF54H20 PDK you want to program is connected.

   .. code-block:: console

      west flash

The sample will be automatically built and programmed on both the Application core and the Peripheral Processor (PPR) of the nRF54H20.

.. _nrf54h20_sample_reading_logs:

Reading the logs
****************

With the :ref:`multicore_hello_world` sample programmed, the nRF54H20 PDK outputs logs for the application core and the peripheral processor.
The logs are output over UART.

To read the logs from the :ref:`multicore_hello_world` sample programmed to the nRF54H20 PDK, complete the following steps:

1. Connect to the PDK with a terminal emulator (for example, `Serial Terminal from nRF Connect for Desktop`_) using the following settings:

   * Baud rate: 115200
   * 8 data bits
   * 1 stop bit
   * No parity
   * HW flow control: None

#. Press the **Reset** button on the PCB to reset the PDK.
#. Observe the console output for both cores:

   * For the application core, the output should be as follows:

     .. code-block:: console

        *** Booting Zephyr OS build v2.7.99-ncs1-2193-gd359a86abf14  ***
        Hello world from nrf54h20dk_nrf54h20_cpuapp

   * For the PPR core, the output should be as follows:

     .. code-block:: console

        *** Booting Zephyr OS build v2.7.99-ncs1-2193-gd359a86abf14  ***
        Hello world from nrf54h20dk_nrf54h20_cpuppr

.. note::
   If no output is shown when using nRF Serial Terminal, select a different serial port in the terminal application.

See the :ref:`ug_nrf54h20_logging` page for more information.

Install |nRFVSC|
****************

To open and compile projects in the |NCS| for the initial limited sampling of the nRF54H20, you can now install and use also the |nRFVSC|.

.. _installing_vsc:

|vsc_extension_description|
For installation and migration instructions, see `How to install the extension`_.

.. note::
   After the installation of both Visual Studio Code and the |nRFVSC| extension, you have to manually point Visual Studio Code to the folder where nrfutil is installed.
   To do so, manually set the ``nrf-connect.nrfutil.home`` option in the user settings of Visual Studio Code.
   Usually, the location is :file:`${env:HOME}/.nrfutil` on macOS and Linux, or :file:`${env:USERPROFILE}/.nrfutil` on Windows.

For other instructions related to the |nRFVSC|, see the `nRF Connect for Visual Studio Code`_ documentation site.

Next steps
**********

You are now all set to use the nRF54H20 PDK.
See the following links for where to go next:

* :ref:`ug_nrf54h20_architecture` for information about the multicore System-on-Chip, such as the responsibilities of the cores and their interprocessor interactions, the memory mapping, and the boot sequence.
* :ref:`ug_nrf54h20_app_samples` to see the available samples for the nRF54H20 PDK for the initial limited sampling.
* The :ref:`introductory documentation <getting_started>` for more information on the |NCS| and the development environment.
