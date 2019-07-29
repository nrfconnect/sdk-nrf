.. _gs_installing_windows:

.. |os| replace:: Windows
.. |installextract| replace:: Install
.. |system_vars| replace:: add them as system environment variables or
.. |install_user| replace:: install
.. |tcfolder| replace:: c:\\gnuarmemb
.. |tcfolder_cc| replace:: ``c:\gnuarmemb``
.. |bash| replace:: command prompt
.. |envfile| replace:: ``zephyr\zephyr-env.cmd``
.. |rcfile| replace:: ``%userprofile%\zephyrrc.cmd``
.. |setexport| replace:: set


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

.. toolchain_start

Installing the toolchain
************************

To be able to cross-compile your applications for Arm targets, you must install  version 7-2018-q2-update of the `GNU Arm Embedded Toolchain`_.

.. important::
   Make sure to install the version that is mentioned above.
   Other versions might not work with the nRF Connect SDK.

To set up the toolchain, complete the following steps:

.. _toolchain_setup:

1. Download the `GNU Arm Embedded Toolchain`_ for your operating system.
#. |installextract| the toolchain into a folder of your choice.
   Make sure that the folder name does not contain any spaces or special characters.
   We recommend to use the folder |tcfolder_cc|.
#. If you want to build and program applications from the command line, define the environment variables for the GNU Arm Embedded toolchain.
   To do so, open a |bash| and enter the following commands (assuming that you have installed the toolchain to |tcfolder_cc|; if not, change the value for GNUARMEMB_TOOLCHAIN_PATH):

   .. parsed-literal::
      :class: highlight

       |setexport| ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
       |setexport| GNUARMEMB_TOOLCHAIN_PATH=\ |tcfolder|

#. Instead of setting the environment variables every time you open a |bash|, |system_vars| define them in the |rcfile| file as described in `Setting up the build environment`_.

.. toolchain_end

.. _cloning_the_repositories_win:

.. cloning_start

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

.. parsed-literal::
   :class: highlight

   pip3 |install_user| west

You only need to do this once.
Like any other Python package, the west bootstrapper is updated regularly.
Therefore, remember to regularly check for updates:

.. parsed-literal::
   :class: highlight

   pip3 |install_user| -U west

Cloning the repositories
========================

.. tip::
   If you already cloned the |NCS| repositories in Git and want to continue using these clones instead of creating new ones, see `Updating your existing clones to use west`_.

To clone the repositories, complete the following steps:

1. Create a folder named ``ncs``.
   This folder will hold all |NCS| repositories.
#. Open a |bash| in the ``ncs`` folder.
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

   This will clone the manifest repository `fw-nrfconnect-nrf`_ into :file:`nrf`.
#. Enter the following command to clone the project repositories::

      west update

Your directory structure now looks like this::

   ncs
    |___ .west
    |___ mcuboot
    |___ nrf
    |___ nrfxlib
    |___ zephyr
    |___ ...


Updating the repositories
=========================

If you work with a specific release of the |NCS|, you do not need to update your repositories, because the release will not change.
However, you might want to switch to a newer release or check out the latest state of development.

To manage the ``nrf`` repository (the manifest repository), use Git.
Checking out a branch or tag in the ``nrf`` repository gives you a different version of the manifest file.
Running ``west update`` will then update the project repositories to the state specified in this manifest file.

For example, to switch to release v0.4.0 of the |NCS|, enter the following commands in the ``ncs/nrf`` directory::

   git checkout v0.4.0
   west update

To switch to the latest state of development, enter the following commands::

   git checkout master
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
    |___ mcuboot
    |___ nrf
    |___ nrfxlib
    |___ zephyr
    |___ ...

Then complete the following steps:

1. Open a |bash| in the ``ncs\nrf`` folder.
#. Do a ``git pull`` or rebase your branch so that you are on the latest fw-nrfconnect-nrf master.
#. Navigate one folder level up to the ``ncs`` folder::

      cd ..

#. Initialize west with the manifest folder from the current branch of your ``nrf`` repository::

      west init -l nrf

   This will create the required ``.west`` folder that is linked to the manifest repository (``nrf``).
#. Enter the following command to clone or update the project repositories::

      west update

.. cloning_end

.. _additional_deps_win:

.. add_deps_start

Installing additional Python dependencies
*****************************************

Both Zephyr and the |NCS| require additional Python packages to be installed.

To install those, open a |bash| in the ``ncs`` folder and enter the following commands:

.. parsed-literal::
   :class: highlight

   pip3 |install_user| -r zephyr/scripts/requirements.txt
   pip3 |install_user| -r nrf/scripts/requirements.txt
   pip3 |install_user| -r mcuboot/scripts/requirements.txt

.. add_deps_end


.. _build_environment_win:

.. buildenv_start

Setting up the build environment
********************************

If you want to build and program your applications from the command line, you must set up your build environment by defining the required environment variables every time you open a new |bash|.
If you plan to :ref:`build with SEGGER Embedded Studio <gs_programming>`, you can skip this step.

To define the environment variables, navigate to the ``ncs`` folder and enter the following command: |envfile|

If you need to define additional environment variables, create the file |rcfile| and add the variables there.
This file is loaded automatically when you run the above command.

.. buildenv_end
