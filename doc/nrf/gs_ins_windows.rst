.. _gs_installing_windows:

.. |os| replace:: Windows

.. intro_start

Installing on |os|
##################

To manually install the |NCS|, you must ensure that all required tools are installed and clone the |NCS| repositories.
See the following sections for detailed instructions.

The first two steps, `Installing the required tools`_ and `Installing the toolchain`_, are identical to the installation in Zephyr.
If you already have your system set up to work with the Zephyr OS, you can skip these steps.

.. intro_end

.. _gs_installing_tools_win:

Installing the required tools
*****************************

The recommended way for installing the required tools on Windows is to use Chocolatey, a package manager for Windows.
Chocolatey installs the tools so that you can use them from a Windows command prompt.

To install the required tools, follow Zephyr's Getting Started Guide.
Note that there are several ways to install the tools.
We recommend installing according to :ref:`zephyr:windows_install_native`.

.. _gs_installing_toolchain_win:

.. |installextract| replace:: Install
.. |tcfolder| replace:: ``c:\gnuarmemb``

.. toolchain_start

Installing the toolchain
************************

To be able to cross-compile your applications for Arm targets, you must install  the `GNU Arm Embedded Toolchain`_.

To set up the toolchain, complete the following steps:

.. _toolchain_setup:

1. Download the `GNU Arm Embedded Toolchain`_ for your operating system.
#. |installextract| the toolchain into a folder of your choice.
   Make sure that the folder name does not contain any spaces or special characters.
   We recommend to use the folder |tcfolder|.
#. If you want to build and program applications from the command line, define the environment variables for the GNU Arm Embedded toolchain.
   To do so, open a command prompt and enter the following commands (assuming that you have installed the toolchain to |tcfolder|; if not, change the value for GNUARMEMB_TOOLCHAIN_PATH)::

     set ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
     set GNUARMEMB_TOOLCHAIN_PATH=c:\gnuarmemb

#. Instead of setting the environment variables every time you open a command prompt, add them as system environment variables or define them in the ``zephyrrc.cmd`` file as described in `Setting up the build environment`_.

.. _cloning_the_repositories_win:

.. |bash| replace:: GIT bash

.. cloning_start

Cloning the repositories
************************

The |NCS| consists of the following repositories:

* |ncs_zephyr_repo|
* |ncs_repo|
* |ncs_mcuboot_repo|
* |ncs_nrfxlib_repo|

.. note::
   The following instructions explain how to clone the repositories to be able to develop your own applications based on the supplied libraries and samples.
   If you want to actively contribute to the |NCS| development and submit the changes that you make, skip this step and :ref:`fork and clone the repositories <gs_installing_forking>` instead.

To clone the repositories, complete the following steps:

1. Create a folder named ``ncs``.
   This folder will hold all |NCS| repositories.
#. Open a |bash| in the ``ncs`` folder.
#. Enter the following commands to clone the repositories::

      git clone https://github.com/NordicPlayground/fw-nrfconnect-zephyr.git zephyr

      git clone https://github.com/NordicPlayground/fw-nrfconnect-mcuboot.git mcuboot

      git clone https://github.com/NordicPlayground/fw-nrfconnect-nrf.git nrf

      git clone https://github.com/NordicPlayground/nrfxlib.git nrfxlib

.. dirstructure_start

Your directory structure now looks like this::

   ncs
    |___ mcuboot
    |___ nrf
    |___ nrfxlib
    |___ zephyr

.. dirstructure_end

The latest state of development is on the master branches of the repositories.
Note that this might not be a stable state.
Therefore, unless you are familiar with the development process, you should always work with a specific release of the |NCS|.

To do so, go to the :ref:`release_notes` of the latest release and check which tags to use for each of the repositories.
Then go to each repository in turn and check out the respective tag.

For example, to check out the v0.3.0 tag for the nrf repository, you would enter the following commands::

   cd nrf
   git checkout tags/v0.3.0
   cd ..

.. cloning_end

.. _additional_deps_win:

.. |prompt| replace:: command prompt

.. add_deps_start

Installing additional Python dependencies
*****************************************

Both Zephyr and the |NCS| require additional Python packages to be installed.

To install those, open a |prompt| in the ``ncs`` folder and enter the following commands:

.. add_deps_end

.. code-block:: console

   pip3 install -r zephyr/scripts/requirements.txt
   pip3 install -r nrf/scripts/requirements.txt


.. _build_environment_win:

.. |envfile| replace:: ``zephyr\zephyr-env.cmd``
.. |rcfile| replace:: ``%userprofile%\zephyrrc.cmd``

.. buildenv_start

Setting up the build environment
********************************

If you want to build and program your applications from the command line, you must set up your build environment by defining the required environment variables every time you open a new |prompt|.

To do so, navigate to the ``ncs`` folder and enter the following command: |envfile|

If you need to define additional environment variables, create the file |rcfile| and add the variables there.
This file is loaded automatically when you run the command above.

.. buildenv_end
